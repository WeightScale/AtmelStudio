#include <FS.h>
#include <ArduinoJson.h>
#include "Scale.h"

ScaleClass Scale(16,14);		/*  gpio16 gpio0  */

ScaleClass::ScaleClass(byte dout, byte pd_sck) : HX711(dout, pd_sck) , ExponentialFilter<long>(){
	_server = NULL;
	_username = NULL;
	_password = NULL;
	_authenticated = false;	
}

ScaleClass::~ScaleClass(){}
	
void ScaleClass::setup(BrowserServerClass *server, const char * username, const char * password){
	init();
	_server = server;
	_username = (char *)username;
	_password = (char *)password;
	
	_server->on(PAGE_FILE, [this]() {								/* Открыть страницу калибровки.*/
		if (!_server->isAuthentified())
			return _server->requestAuthentication();
		saveValueCalibratedHttp();
	});
	_server->on("/av", [this](){									/* Получить значение АЦП усредненное. */
		_server->send(200, "text/html", String(readAverage()));
	});
	_server->on("/tp", [this](){									/* Установить тару. */
		tare();
		_server->send(204, "text/html", "");
	});
	_server->on("/sl", [this](){									/* Опломбировать */		
		if (saveDate()){
			_server->send(200, "text/html", "");
		}
	});
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
		
		ok:
			return _server->send(200, "text/html", "");
		err:
			return _server->send(400, "text/html", "Ошибка ");	
	}		
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
	_scales_value.max = json["max_weight_id"];
	_scales_value.offset = json["offset"];
	setAverage(json["average_id"]);
	_scales_value.step = json["step_id"];
	_scales_value.accuracy = json["accuracy_id"];
	_scales_value.scale = json["scale"];
	SetFilterWeight(json["filter_id"]);
	return true;
	
}

bool ScaleClass::saveDate() {
	File cdateFile = SPIFFS.open(CDATE_FILE, "w+");
	if (!cdateFile) {
		cdateFile.close();
		return false;
	}
	//size_t size = cdateFile.size();
	//std::unique_ptr<char[]> buf(new char[size]);
	//cdateFile.readBytes(buf.get(), size);
	//DynamicJsonBuffer jsonBuffer(size);
	//JsonObject& json = jsonBuffer.parseObject(buf.get());
	DynamicJsonBuffer jsonBuffer;
	JsonObject& json = jsonBuffer.createObject();

	if (!json.success()) {
		return false;
	}
	
	json["step_id"] = _scales_value.step;
	json["average_id"] = _scales_value.average;
	json["max_weight_id"] = _scales_value.max;
	json["offset"] = _scales_value.offset;
	json["accuracy_id"] = _scales_value.accuracy;
	json["scale"] = _scales_value.scale;
	json["filter_id"] = GetFilterWeight();

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

d_type ScaleClass::getUnits() {
	d_type v = getValue();
	return (v * _scales_value.scale);
}

d_type ScaleClass::getWeight(){
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
void ScaleClass::formatValue(d_type value, char* string){
	dtostrf(value, 6-_scales_value.accuracy, _scales_value.accuracy, string);
}



