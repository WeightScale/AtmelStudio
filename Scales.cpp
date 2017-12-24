#include <EEPROM.h>
#include <ESP8266HTTPClient.h>
#include <FS.h>
#include <ArduinoJson.h>
#include "Scales.h"
#include "DateTime.h"
#include "BrowserServer.h"

ScalesClass SCALES(16,14);							/*  gpio16 gpio0  */

ScalesClass::ScalesClass(byte dout, byte pd_sck) : HX711(dout, pd_sck){
}

ScalesClass::~ScalesClass(){}

void ScalesClass::begin(){	
	//ScaleMemClass::init();
	loadSettings();	
	
	reset();
	set_step(_settings.step);
	set_filter(_settings.filter);
	set_scale(_settings.scale);
	set_offset(_settings.adc_offset);
	ADCFilter.SetCurrent(_settings.adc_offset);
	mathRound();
	//updateSettings();
	//tare();	
}

void ScalesClass::setSSID(const String& ssid){
	_settings.scaleWlanSSID = ssid;
	//ssid.toCharArray(_settings.ssid, sizeof(_settings.ssid));
}

void ScalesClass::setPASS(const String& pass){
	_settings.scaleWlanKey = pass;
	//pass.toCharArray(_settings.key, sizeof(_settings.key));
}

bool ScalesClass::saveEvent(const String& event, const String& value) {
	File readFile = SPIFFS.open("/events.json", "r");
    if (!readFile) {        
        readFile.close();
		if (!SPIFFS.exists("/events.json")){
			readFile = SPIFFS.open("/events.json", "w+");	
		}else{
			return false;	
		}
    }
	String date = getDateTime();
    size_t size = readFile.size(); 
	
	bool flag = WiFi.status() == WL_CONNECTED?SCALES.eventToServer(date, event, value):false;
	
    std::unique_ptr<char[]> buf(new char[size]);
    readFile.readBytes(buf.get(), size);	
    readFile.close();
		
    DynamicJsonBuffer jsonBuffer(JSON_ARRAY_SIZE(110));
	JsonObject& json = jsonBuffer.parseObject(buf.get());

    if (!json.containsKey("events")) {	
		json["cur_num"] = 0;
		json["max_events"] = MAX_EVENTS;
		JsonArray& events = json.createNestedArray("events");
		for(int i = 0; i < MAX_EVENTS; i++){
			JsonObject& ev = jsonBuffer.createObject();
			ev["date"] = "";
			ev["value"] = "";
			ev["server"] = false;
			events.add(ev);	
		}		
		/*if (!json.success())
			return false;*/
    }
	
	long n = json["cur_num"];
	
	json["events"][n]["date"] = date;
	json["events"][n]["value"] = value;	
	json["events"][n]["server"] = flag;
		
	if ( ++n == MAX_EVENTS){
		n = 0;
	}
	json["max_events"] = MAX_EVENTS;
	json["cur_num"] = n;
	//TODO add AP data to html
	File saveFile = SPIFFS.open("/events.json", "w");
	if (!saveFile) {
		saveFile.close();
		return false;
	}

	json.printTo(saveFile);
	saveFile.flush();
	saveFile.close();
	return true;
}

/*
JsonObject &ScalesClass::createEventsJson(){
	DynamicJsonBuffer jsonBuffer(JSON_ARRAY_SIZE(110));
	JsonObject &json = jsonBuffer.createObject();
	json["cur_num"] = 0;
	json["max_events"] = 100;
	JsonArray& events = json.createNestedArray("events");
	for(int i = 0; i < 100; i++){
		JsonObject& ev = jsonBuffer.createObject();
		ev["date"] = "";
		ev["value"] = "";
		ev["server"] = "";
		events.add(ev);	
	}	
	return json;
}*/

/*
bool ScalesClass::pingServer(){
	HTTPClient http;
	http.begin("http://viktyusk.dp.ua/scales.php?ping");
	http.setTimeout(500);
	int httpCode = http.GET();
	http.end();
	if(httpCode == HTTP_CODE_OK) {
		return true;
	}
	return false;	
}
*/

String ScalesClass::getIp() {
	WiFiClient client ;
	String ip = "";
	if (client.connect("api.ipify.org", 80)) {
		client.println("GET / HTTP/1.0");
		client.println("Host: api.ipify.org");
		client.println();
		delay(50);
		int a=1;
		while (client.connected()) {
			++a;
			String line = client.readStringUntil('\n');
			if (a == 11) {
				ip = line;
				client.stop();
			}
		}
	}
	client.stop();	
	return ip;
}

/*
String ScalesClass::getIp(){
	
	HTTPClient http;	
	http.begin("http://http://ip.jsontest.com/");
	http.setTimeout(2000);	
	int httpCode = http.GET();
	String ip = "";	
	if(httpCode == HTTP_CODE_OK){		
		ip = http.getString();
	}
	http.end();
	return ip;
}*/

/*
	
	http://viktyusk.dp.ua/scales.php?email=
	
	http://viktyusk.dp.ua/scales.php?email=
	
	http://viktyusk.dp.ua/scales.php?code=...&date=...&type=...&value=...
	
	http://viktyusk.dp.ua/scales.php?code=...&size=...&page=...	
	
	http://viktyusk.dp.ua/scales.php?email=
	
	http://viktyusk.dp.ua/scales.php?email=
*/

/* */	
bool ScalesClass::eventToServer(const String& date, const String& type, const String& value){
	HTTPClient http;
	String message = "http://";
	message += _settings.hostUrl.c_str();
	String hash = getHash(_settings.hostPin.c_str(), date, type, value);
	//Serial.println("Hash="+hash);
/*
	message += "?code=" + _settings.hostPin + "&";
	message += "date=" + date + "&";
	message += "type=" + type + "&";
	message += "value=" + value;
*/
	message += "/scales.php?hash=" + hash;
	http.begin(message);
	http.setTimeout(1000);
	int httpCode = http.GET();
	http.end();
	if(httpCode == HTTP_CODE_OK) {
		return true;
	}
	return false;
}

void ScalesClass::sendScaleSettingsSaveValue() {
	//if (browserServer.args() > 0){  // Save Settings
		bool flag = false;
		for (uint8_t i = 0; i < browserServer.args(); i++) {
			if (browserServer.argName(i)=="date"){
				DateTimeClass DateTime(browserServer.arg("date"));
				Rtc.SetDateTime(DateTime.toRtcDateTime());
				String message = "<div>Дата время установлена<br/>";
				message+=getDateTime()+"</div>";
				browserServer.send(200, "text/html", message);
				return;
			}
			if (browserServer.argName(i) == "host") {
				_settings.hostUrl = browserServer.urldecode(browserServer.arg(i));
				flag = true;
				//continue;
			}else if (browserServer.argName(i) == "pin") {
				_settings.hostPin = browserServer.urldecode(browserServer.arg(i));
				flag = true;
				//continue;
			}else if (browserServer.argName(i) == "name_admin") {
				_settings.scaleName = browserServer.urldecode(browserServer.arg(i));
				flag = true;
				//continue;
			}else if (browserServer.argName(i) == "pass_admin") {
				_settings.scalePass = browserServer.urldecode(browserServer.arg(i));
				flag = true;
				//continue;
			}else if (browserServer.argName(i) == "ssid") {
				_settings.scaleWlanSSID = browserServer.urldecode(browserServer.arg(i));
				flag = true;
				//continue;
			}else if (browserServer.argName(i) == "key") {
				_settings.scaleWlanKey = browserServer.urldecode(browserServer.arg(i));
				connect = _settings.scaleWlanKey.length() > 0;
				flag = true;
				//continue;
			}
		}
		if(flag){
			if (saveSettings()){
				//browserServer.send(200, "text/html", "");
				handleFileRead(browserServer.uri());
			}else{
				browserServer.send(400, "text/html", "РћС€РёР±РєР°");
			}
		}
	//}
}

void ScalesClass::scaleCalibrateSaveValue() {
	//if (browserServer.args() > 0){  // Save Settings
		bool flag = false;
		for (uint8_t i = 0; i < browserServer.args(); i++) {			
			if (browserServer.argName(i) == "weightMax") {				
				_settings.max = browserServer.arg(i).toInt();
				flag = true;
			}if (browserServer.argName(i) == "weightStep") {
				_settings.step = browserServer.arg(i).toInt();
				flag = true;
			}if (browserServer.argName(i) == "weightAccuracy") {
				_settings.accuracy = browserServer.arg(i).toInt();
				flag = true;
			}if (browserServer.argName(i) == "weightFilter") {
				_settings.filter = browserServer.arg(i).toInt();
				flag = true;
			}if (browserServer.argName(i) == "weightFilter1") {
				ADCFilter.SetWeight(browserServer.arg(i).toInt());
				flag = true;
			}else if (browserServer.argName(i) == "zero") {
				calibrateZero = read_average(_settings.filter);				
				flag = true;
			}else if (browserServer.argName(i) == "weightCal") {
				accurateWeight = browserServer.arg(i).toFloat();
				calibrateWeight = read_average(_settings.filter);
				mathScale();							
				set_scale(_settings.scale);
				flag = true;
			}
		}
		if (browserServer.hasArg("update")){
			if (SCALES.saveDate()){
				SCALES.updateSettings();				
			}
		}		
		if(flag){
			browserServer.send(200, "text/html", "");
			mathRound();			
		}else{
			browserServer.send(400, "text/html", "Ошибка°");
		}
	//}
}

String ScalesClass::getHash(const String& code, const String& date, const String& type, const String& value){
	
	String event = String(code);
	event += "\t" + date + "\t" + type + "\t" + value;
	int s = 0;
	for (int i = 0; i < event.length(); i++)
		s += event[i];
	event += (char) (s % 256);
	String hash = "";
	for (int i = 0; i < event.length(); i++){
		int c = (event[i] - (i == 0? 0 : event[i - 1]) + 256) % 256;
		int c1 = c / 16; int c2 = c % 16;
		char d1 = c1 < 10? '0' + c1 : 'a' + c1 - 10;
		char d2 = c2 < 10? '0' + c2 : 'a' + c2 - 10;
		hash += "%"; hash += d1; hash += d2;
	} 
	return hash;
}

int ScalesClass::getBattery(int times){
	long sum = 0;
	for (byte i = 0; i < times; i++) {
		sum += analogRead(A0);
	}
	return times == 0?sum :sum / times;	
}

void ScalesClass::mathScale(){
	//_settings.scale = (double)((double)accurateWeight / (double)((double)calibrateWeight - (double)calibrateZero));
	_settings.scale = calibrateWeight - calibrateZero;
	//_settings.scale = (double)((double)((double)calibrateWeight - (double)calibrateZero)/(double)accurateWeight );
	_settings.scale /= accurateWeight;
	_settings.adc_offset = calibrateZero;
}

bool ScalesClass::saveSettings() {
	File readFile = SPIFFS.open(SETTINGS_FILE, "r");
	if (!readFile) {
		readFile.close();
		return false;
	}
	size_t size = readFile.size();
	std::unique_ptr<char[]> buf(new char[size]);
	readFile.readBytes(buf.get(), size);
	readFile.close();
	DynamicJsonBuffer jsonBuffer(size);
	//StaticJsonBuffer<256> jsonBuffer;
	JsonObject& json = jsonBuffer.parseObject(buf.get());
	//JsonObject& json = openJsonFile(SERVER_FILE);

	if (!json.success()) {
		return false;
	}
	
	json["scale"]["id_name_admin"] = _settings.scaleName;
	json["scale"]["id_pass_admin"] = _settings.scalePass;
	json["scale"]["id_ssid"] = _settings.scaleWlanSSID;
	json["scale"]["id_key"] = _settings.scaleWlanKey;
	
	json["server"]["id_host"] = _settings.hostUrl;
	json["server"]["id_pin"] = _settings.hostPin;

	//TODO add AP data to html
	File serverFile = SPIFFS.open(SETTINGS_FILE, "w");
	if (!serverFile) {
		serverFile.close();
		return false;
	}

	json.printTo(serverFile);
	serverFile.flush();
	serverFile.close();
	return true;
}

bool ScalesClass::saveDate() {
	File readFile = SPIFFS.open(SETTINGS_FILE, "r");
	if (!readFile) {
		readFile.close();
		return false;
	}
	size_t size = readFile.size();
	std::unique_ptr<char[]> buf(new char[size]);
	readFile.readBytes(buf.get(), size);
	readFile.close();
	DynamicJsonBuffer jsonBuffer(size);
	//StaticJsonBuffer<256> jsonBuffer;
	JsonObject& json = jsonBuffer.parseObject(buf.get());
	//JsonObject& json = openJsonFile(SERVER_FILE);

	if (!json.success()) {
		return false;
	}
	
	json["date"]["filter_id"] = _settings.filter;
	json["date"]["step_id"] = _settings.step;
	json["date"]["accuracy_id"] = _settings.accuracy;
	json["date"]["max_weight_id"] = _settings.max;
	json["date"]["adc_offset"] = _settings.adc_offset;
	json["date"]["scale"] = _settings.scale;
	json["date"]["wfilter_id"] = ADCFilter.GetWeight();	

	//TODO add AP data to html
	File saveFile = SPIFFS.open(SETTINGS_FILE, "w");
	if (!saveFile) {
		saveFile.close();
		return false;
	}

	json.printTo(saveFile);
	saveFile.flush();
	saveFile.close();
	return true;
}

bool ScalesClass::loadSettings() {
	File serverFile = SPIFFS.open(SETTINGS_FILE, "r");
	if (!serverFile) {
		_settings.scaleName = "admin";
		_settings.scalePass = "1234";
		_settings.scaleWlanSSID = "";
		_settings.scaleWlanKey = "";
		_settings.hostUrl = "";
		_settings.hostPin = "";
		_settings.filter = 1;
		_settings.step = 1;
		_settings.accuracy = 0;
		_settings.adc_offset = 0;
		_settings.max = 1000;
		_settings.scale = 1;
		ADCFilter.SetWeight(20);
		serverFile.close();
		return false;
	}

	size_t size = serverFile.size();

	// Allocate a buffer to store contents of the file.
	std::unique_ptr<char[]> buf(new char[size]);

	// We don't use String here because ArduinoJson library requires the input
	// buffer to be mutable. If you don't use ArduinoJson, you may as well
	// use configFile.readString instead.
	serverFile.readBytes(buf.get(), size);
	serverFile.close();
	DynamicJsonBuffer jsonBuffer(size);
	//StaticJsonBuffer<256> jsonBuffer;
	JsonObject& json = jsonBuffer.parseObject(buf.get());

	if (!json.success()) {
		_settings.scaleName = "admin";
		_settings.scalePass = "1234";		
		return false;
	}	
	_settings.scaleName = json["scale"]["id_name_admin"].asString();
	_settings.scalePass = json["scale"]["id_pass_admin"].asString();
	_settings.scaleWlanSSID = json["scale"]["id_ssid"].asString();
	_settings.scaleWlanKey = json["scale"]["id_key"].asString();
	_settings.hostUrl = json["server"]["id_host"].asString();
	_settings.hostPin = json["server"]["id_pin"].asString();
	_settings.max = json["date"]["max_weight_id"];
	_settings.adc_offset = json["date"]["adc_offset"];
	_settings.filter = json["date"]["filter_id"];
	_settings.step = json["date"]["step_id"];
	_settings.accuracy = json["date"]["accuracy_id"];
	_settings.scale = json["date"]["scale"];
	ADCFilter.SetWeight(json["date"]["wfilter_id"]);
	return true;
}

/* */
void ScalesClass::detectStable(){
	if(abs(_weight) > 10){
		if (_weight_temp /*- _settings.step*/ <= _weight && _weight_temp /*+ _settings.step*/ >= _weight ) {
			if (_stable_num <= STABLE_NUM_MAX){
				if (_stable_num == STABLE_NUM_MAX) {
					if (!isStable){
						saveEvent("weight", String(_weight)+"_kg");
						isStable = true;
					}
					return;
				}
				_stable_num++;
			}
		} else {
			_stable_num=0;
			isStable = false;
		}
		_weight_temp = _weight;
	}
}

void ScalesClass::updateSettings(){
	set_step(_settings.step);
	set_filter(_settings.filter);
	set_scale(_settings.scale);
	//set_offset(_settings.adc_offset);
}

/*! Функция для форматирования значения веса
	value - Форматируемое значение
	digits - Количество знаков после запятой
	accuracy - Точновть шага значений (1, 2, 5, ...)
	string - Выходная строка отфармотронова значение 
*/
void ScalesClass::formatValue(d_type value, /*int digits, int accuracy,*/ char* string){
	//d_type r = pow(10.0, _settings.accuracy) / _settings.step; // РјРЅРѕР¶РёС‚РµР»СЊ РґР»СЏ РѕРєСЂСѓРіР»РµРЅРёСЏ
	//dtostrf(round(value * _round) / _round, 6-_settings.accuracy, _settings.accuracy, string);
	dtostrf(value, 6-_settings.accuracy, _settings.accuracy, string);
}

void powerOff(){
	SCALES.power_down(); /// Выключаем ацп
	digitalWrite(EN_NCP, LOW); /// Выключаем стабилизатор
}

//==============================Filter================================================
/*
d_type ScalesClass::filter(d_type data, / *d_type prev_data,* / d_type delta_data, d_type filter_step, d_type filter_cof){
	d_type temp,temp1;
	static d_type filter_count = 1, prev_data;
	
	if((data - prev_data) > delta_data || (prev_data - data) > delta_data)
		filter_count = 1;
	else
		if((filter_count - filter_step) > filter_cof)
			filter_count -= filter_step;
		else
			filter_count = filter_cof;
	temp = data *  filter_count;
	temp1 = (1- filter_count) * prev_data;
	prev_data = temp +temp1;
	//temp += (1- filter_count) * prev_weight;
	return prev_data;
}*/




