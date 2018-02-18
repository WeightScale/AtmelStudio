#include <FS.h>
#include <ArduinoJson.h>
#include "Scale.h"

ScaleClass Scale(16,14);		/*  gpio16 gpio0  */

ScaleClass::ScaleClass(byte dout, byte pd_sck) : HX711(dout, pd_sck) , ExponentialFilter<long>(){
	_server = NULL;	
	_authenticated = false;	
}

ScaleClass::~ScaleClass(){}
	
void ScaleClass::setup(BrowserServerClass *server){
	init();
	_server = server;
	_server->on("/wt",HTTP_GET, handleWeight);						/* Получить вес и заряд. */
	_server->on(PAGE_FILE, [this]() {								/* Открыть страницу калибровки.*/
		if(!_server->authenticate(_scales_value.user.c_str(), _scales_value.password.c_str()))
			return _server->requestAuthentication();
		_authenticated = true;
		saveValueCalibratedHttp();
	});
	_server->on("/av", [this](){									/* Получить значение АЦП усредненное. */
		_server->send(200, TEXT_HTML, String(readAverage()));
	});
	_server->on("/tp", [this](){									/* Установить тару. */
		tare();
		_server->send(204, TEXT_HTML, "");
	});
	_server->on("/sl", handleSeal);									/* Опломбировать */	
}

void ScaleClass::init(){
	reset();
	_downloadValue();
	mathRound();
	SetCurrent(readAverage());
}

void ScaleClass::mathRound(){
	_round = pow(10.0, _scales_value.accuracy) / _scales_value.step; // множитель для округления}
	_stable_step = 1/_round;
}

void ScaleClass::saveValueCalibratedHttp() {
	if (_server->args() > 0) {	
		if (_server->hasArg("update")){
			_scales_value.accuracy = _server->arg("weightAccuracy").toInt();
			_scales_value.step = _server->arg("weightStep").toInt();
			setAverage(_server->arg("weightAverage").toInt());
			SetFilterWeight(_server->arg("weightFilter").toInt());
			_scales_value.max = _server->arg("weightMax").toInt();
			mathRound();
			if (saveDate()){
				goto ok;
			}
			goto err;
		}
		
		if (_server->hasArg("zero")){
			_scales_value.offset = readAverage();
		}
		
		if (_server->hasArg("weightCal")){
			_referenceWeight = _server->arg("weightCal").toFloat();			
			_calibrateWeightValue = readAverage();
			mathScale();
		}
		
		if (_server->hasArg("user")){
			_scales_value.user = _server->arg("user");
			_scales_value.password = _server->arg("pass");
			if (saveDate()){
				goto url;
			}
			goto err;
		}
		
		ok:
			return _server->send(200, TEXT_HTML, "");
		err:
			return _server->send(400, TEXT_HTML, "Ошибка ");	
	}
	url:		
	handleFileRead(_server->uri());
	
	//}
}

bool ScaleClass::_downloadValue(){
	_scales_value.average = 1;
	_scales_value.step = 1;
	_scales_value.accuracy = 0;
	_scales_value.offset = 0;
	_scales_value.max = 1000;
	_scales_value.scale = 1;
	SetFilterWeight(80);
	_scales_value.user = "admin";
	_scales_value.password = "1234";
	File dateFile;
	if (SPIFFS.exists(CDATE_FILE)){
		dateFile = SPIFFS.open(CDATE_FILE, "r");
	}else{
		dateFile = SPIFFS.open(CDATE_FILE, "w+");
	}
	if (!dateFile) {
		dateFile.close();
		return false;
	}
	size_t size = dateFile.size();
		
	std::unique_ptr<char[]> buf(new char[size]);
		
	dateFile.readBytes(buf.get(), size);
	dateFile.close();
	DynamicJsonBuffer jsonBuffer(size);
	JsonObject& json = jsonBuffer.parseObject(buf.get());

	if (!json.success()) {
		return false;
	}
	_scales_value.max = json[WEIGHT_MAX_JSON];
	_scales_value.offset = json[OFFSET_JSON];
	setAverage(json[AVERAGE_JSON]);
	_scales_value.step = json[STEP_JSON];
	_scales_value.accuracy = json[ACCURACY_JSON];
	_scales_value.scale = json[SCALE_JSON];
	SetFilterWeight(json[FILTER_JSON]);
	_scales_value.seal = json[SEAL_JSON];
	if (!json.containsKey(USER_JSON)){
		_scales_value.user = "admin";
		_scales_value.password = "1234";	
	}else{
		_scales_value.user = json[USER_JSON].as<String>();
		_scales_value.password = json[PASS_JSON].as<String>();
	}
	
	return true;
	
}

bool ScaleClass::saveDate() {
	File cdateFile = SPIFFS.open(CDATE_FILE, "w+");
	if (!cdateFile) {
		cdateFile.close();
		return false;
	}
	DynamicJsonBuffer jsonBuffer;
	JsonObject& json = jsonBuffer.createObject();

	if (!json.success()) {
		return false;
	}
	
	json[STEP_JSON] = _scales_value.step;
	json[AVERAGE_JSON] = _scales_value.average;
	json[WEIGHT_MAX_JSON] = _scales_value.max;
	json[OFFSET_JSON] = _scales_value.offset;
	json[ACCURACY_JSON] = _scales_value.accuracy;
	json[SCALE_JSON] = _scales_value.scale;
	json[FILTER_JSON] = GetFilterWeight();
	json[SEAL_JSON] = _scales_value.seal;
	json[USER_JSON] = _scales_value.user;
	json[PASS_JSON] = _scales_value.password;
	
	json.printTo(cdateFile);
	cdateFile.flush();
	cdateFile.close();
	return true;
}

long ScaleClass::readAverage() {
	long long sum = 0;
	for (byte i = 0; i < _scales_value.average; i++) {
		sum += read();
	}
	return _scales_value.average == 0?sum / 1:sum / _scales_value.average;
}

long ScaleClass::getValue() {
	Filter(readAverage());
	return Current() - _scales_value.offset;
}

float ScaleClass::getUnits() {
	float v = getValue();
	return (v * _scales_value.scale);
}

float ScaleClass::getWeight(){
	return round(getUnits() * _round) / _round; 
}

void ScaleClass::tare() {
	long sum = readAverage();
	setOffset(sum);
}

void ScaleClass::setAverage(unsigned char a){
	_scales_value.average = constrain(a, 1, 5);
}

void ScaleClass::mathScale(){
	_scales_value.scale = _referenceWeight / float(_calibrateWeightValue - _scales_value.offset);
}

/*! Функция для форматирования значения веса
	value - Форматируемое значение
	digits - Количество знаков после запятой
	accuracy - Точновть шага значений (1, 2, 5, ...)
	string - Выходная строка отфармотронова значение 
*/
void ScaleClass::formatValue(float value, char* string){
	dtostrf(value, 6-_scales_value.accuracy, _scales_value.accuracy, string);
}

/* */
void ScaleClass::detectStable(float w){
	static float weight_temp;
	static unsigned char stable_num;
		if (weight_temp == w) {
			//if (stable_num <= STABLE_NUM_MAX){
			if (stable_num > STABLE_NUM_MAX) {
				if (!stableWeight){
					if(abs(w) > _stable_step){
						CORE.saveEvent("weight", String(w)+"_kg");	
					}
					stableWeight = true;
				}
				return;
			}
			stable_num++;
			//}
			} else {
			stable_num = 0;
			stableWeight = false;
		}
		weight_temp = w;
}

void handleSeal(){
	randomSeed(Scale.readAverage());
	Scale.setSeal(random(1000));
	
	if (Scale.saveDate()){
		Scale.getServer()->send(200, TEXT_HTML, String(Scale.getSeal()));
	}
}

void handleWeight(){
	char buffer[10];
	float w = Scale.getWeight();
	Scale.formatValue(w, buffer	);
	Scale.detectStable(w);
	
	taskPower.updateCache();
	Scale.getServer()->send(200, "text/plain", String("{\"w\":\""+String(buffer)+"\",\"c\":"+String(CORE.getCharge())+",\"s\":"+String(Scale.getStableWeight())+"}"));	
}



