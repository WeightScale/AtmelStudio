#include <ESP8266HTTPClient.h>
#include <FS.h>
#include <ArduinoJson.h>
#include "Core.h"
#include "DateTime.h"
#include "BrowserServer.h"

CoreClass CORE;

CoreClass::CoreClass(){
}

CoreClass::~CoreClass(){}

void CoreClass::begin(){
	SPIFFS.begin();
	_downloadSettings();
	Rtc.Begin();
}

void CoreClass::setSSID(const String& ssid){
	_settings.scaleWlanSSID = ssid;
}

void CoreClass::setPASS(const String& pass){
	_settings.scaleWlanKey = pass;
}

bool CoreClass::saveEvent(const String& event, const String& value) {
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
	
	bool flag = WiFi.status() == WL_CONNECTED?CORE.eventToServer(date, event, value):false;
	
    std::unique_ptr<char[]> buf(new char[size]);
    readFile.readBytes(buf.get(), size);	
    readFile.close();
		
    DynamicJsonBuffer jsonBuffer(JSON_ARRAY_SIZE(110));
	JsonObject& json = jsonBuffer.parseObject(buf.get());

    if (!json.containsKey(EVENTS_JSON)) {	
		json["cur_num"] = 0;
		json["max_events"] = MAX_EVENTS;
		JsonArray& events = json.createNestedArray(EVENTS_JSON);
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
	
	json[EVENTS_JSON][n]["date"] = date;
	json[EVENTS_JSON][n]["value"] = value;	
	json[EVENTS_JSON][n]["server"] = flag;
		
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

String CoreClass::getIp() {
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
bool CoreClass::eventToServer(const String& date, const String& type, const String& value){
	HTTPClient http;
	String message = "http://";
	message += _settings.hostUrl.c_str();
	String hash = getHash(_settings.hostPin.c_str(), date, type, value);	
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

void CoreClass::sendScaleSettingsSaveValue() {
	//if (browserServer.args() > 0){  // Save Settings
		bool flag = false;
		String message = " ";
		if (browserServer.hasHeader("x-SETNET")){
			flag = true;
			_settings.autoIp = false;
			if (browserServer.hasArg("auto"))
				_settings.autoIp = true;				 			
			_settings.scaleLanIp = browserServer.urldecode(browserServer.arg("lan_ip"));			
			_settings.scaleGateway = browserServer.urldecode(browserServer.arg("gateway"));
			_settings.scaleSubnet = browserServer.urldecode(browserServer.arg("subnet"));
			_settings.scaleWlanSSID = browserServer.urldecode(browserServer.arg("ssid"));
			_settings.scaleWlanKey = browserServer.urldecode(browserServer.arg("key"));	
			browserServer.send(200, "text/html", "OK");		
			connectWifi();	
			goto out;
		}
		
		for (uint8_t i = 0; i < browserServer.args(); i++) {
			if (browserServer.argName(i)=="date"){
				DateTimeClass DateTime(browserServer.arg("date"));
				Rtc.SetDateTime(DateTime.toRtcDateTime());
				String message = "<div>Дата время установлена<br/>";
				message += getDateTime()+"</div>";
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
			}/*else if (browserServer.argName(i) == "auto") {
				_settings.autoIp = true;
				flag = true;
				//continue;
			}else if (browserServer.argName(i) == "lan_ip") {
				_settings.scaleLanIp = browserServer.urldecode(browserServer.arg(i));
				//if(_settings.scaleWlanSSID.length() > 0)
					//connectWifi();
				flag = true;
				//continue;
			}else if (browserServer.argName(i) == "gateway") {
				_settings.scaleGateway = browserServer.urldecode(browserServer.arg(i));
				//connect = _settings.scaleWlanSSID.length() > 0;
				//if(_settings.scaleWlanSSID.length() > 0)
				//connectWifi();
				flag = true;
				//continue;
			}else if (browserServer.argName(i) == "subnet") {
				_settings.scaleSubnet = browserServer.urldecode(browserServer.arg(i));
				//connect = _settings.scaleWlanSSID.length() > 0;
				//if(_settings.scaleWlanSSID.length() > 0)
				//connectWifi();
				flag = true;
				//continue;
			}else if (browserServer.argName(i) == "ssid") {
				_settings.scaleWlanSSID = browserServer.urldecode(browserServer.arg(i));
				//connect = _settings.scaleWlanSSID.length() > 0;
				if(_settings.scaleWlanSSID.length() > 0)
					connectWifi();
				flag = true;
				//continue;
			}else if (browserServer.argName(i) == "key") {
				_settings.scaleWlanKey = browserServer.urldecode(browserServer.arg(i));
				//connect = _settings.scaleWlanKey.length() > 0;
				if(_settings.scaleWlanSSID.length() > 0)
					connectWifi();
				flag = true;
				//continue;
			}*/
		}
		out:
		if(flag){
			if (_saveSettings()){
				//browserServer.send(200, "text/html", "OK");
				handleFileRead(browserServer.uri());
			}else{
				browserServer.send(400, "text/html", "Ошибка ");
			}
		}
	//}
}

void CoreClass::scaleCalibrateSaveValue() {
	//if (browserServer.args() > 0){  // Save Settings
		bool flag = false;
		for (uint8_t i = 0; i < browserServer.args(); i++) {			
			if (browserServer.argName(i) == "weightMax") {
				Scale.setMax(browserServer.arg(i).toInt());	
				flag = true;
			}if (browserServer.argName(i) == "weightStep") {
				Scale.setStep(browserServer.arg(i).toInt());
				flag = true;
			}if (browserServer.argName(i) == "weightAccuracy") {				
				Scale.setAccuracy(browserServer.arg(i).toInt());
				flag = true;
			}if (browserServer.argName(i) == "weightAverage") {
				Scale.setAverage(browserServer.arg(i).toInt());
				flag = true;
			}if (browserServer.argName(i) == "weightFilter") {
				Scale.SetFilterWeight(browserServer.arg(i).toInt());
				flag = true;
			}else if (browserServer.argName(i) == "zero") {
				Scale.setCalibrateZeroValue(Scale.readAverage());				
				flag = true;
			}else if (browserServer.argName(i) == "weightCal") {
				Scale.setReferenceWeight(browserServer.arg(i).toFloat());
				Scale.setCalibrateWeightValue(Scale.readAverage());
				Scale.mathScale();
				flag = true;
			}
		}
		if (browserServer.hasArg("update")){
			if (Scale.saveDate()){
				//SCALES.updateSettings();				
			}
		}		
		if(flag){
			browserServer.send(200, "text/html", "");
			Scale.mathRound();			
		}else{
			browserServer.send(400, "text/html", "Ошибка ");
		}
	//}
}

String CoreClass::getHash(const String& code, const String& date, const String& type, const String& value){
	
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

int CoreClass::getBattery(int times){
	long sum = 0;
	for (byte i = 0; i < times; i++) {
		sum += analogRead(A0);
	}
	return times == 0?sum :sum / times;	
}

bool CoreClass::_saveSettings() {
	File serverFile = SPIFFS.open(SETTINGS_FILE, "w+");
	if (!serverFile) {
		serverFile.close();
		return false;
	}
	size_t size = serverFile.size();
	std::unique_ptr<char[]> buf(new char[size]);
	serverFile.readBytes(buf.get(), size);
	//readFile.close();
	DynamicJsonBuffer jsonBuffer(size);
	JsonObject& json = jsonBuffer.parseObject(buf.get());

	if (!json.success()) {
		return false;
	}
	
	json[SCALE_JSON]["id_name_admin"] = _settings.scaleName;
	json[SCALE_JSON]["id_pass_admin"] = _settings.scalePass;
	json[SCALE_JSON]["id_auto"] = _settings.autoIp;
	json[SCALE_JSON]["id_lan_ip"] = _settings.scaleLanIp;
	json[SCALE_JSON]["id_gateway"] = _settings.scaleGateway;
	json[SCALE_JSON]["id_subnet"] = _settings.scaleSubnet;
	json[SCALE_JSON]["id_ssid"] = _settings.scaleWlanSSID;
	json[SCALE_JSON]["id_key"] = _settings.scaleWlanKey;
	
	json[SERVER_JSON]["id_host"] = _settings.hostUrl;
	json[SERVER_JSON]["id_pin"] = _settings.hostPin;

	json.printTo(serverFile);
	serverFile.flush();
	serverFile.close();
	return true;
}

bool CoreClass::_downloadSettings() {
	_settings.scaleName = "admin";
	_settings.scalePass = "1234";
	_settings.autoIp = true;
	File serverFile = SPIFFS.open(SETTINGS_FILE, "w+");
	if (!serverFile) {			
		serverFile.close();
		return false;
	}
	size_t size = serverFile.size();

	// Allocate a buffer to store contents of the file.
	std::unique_ptr<char[]> buf(new char[size]);
	
	serverFile.readBytes(buf.get(), size);
	serverFile.close();
	DynamicJsonBuffer jsonBuffer(size);
	JsonObject& json = jsonBuffer.parseObject(buf.get());

	if (!json.success()) {
		return false;
	}	
	_settings.scaleName = json[SCALE_JSON]["id_name_admin"].asString();
	_settings.scalePass = json[SCALE_JSON]["id_pass_admin"].asString();
	_settings.autoIp = json[SCALE_JSON]["id_auto"];
	_settings.scaleLanIp = json[SCALE_JSON]["id_lan_ip"].asString();
	_settings.scaleGateway = json[SCALE_JSON]["id_gateway"].asString();
	_settings.scaleSubnet = json[SCALE_JSON]["id_subnet"].asString();
	_settings.scaleWlanSSID = json[SCALE_JSON]["id_ssid"].asString();
	_settings.scaleWlanKey = json[SCALE_JSON]["id_key"].asString();
	_settings.hostUrl = json[SERVER_JSON]["id_host"].asString();
	_settings.hostPin = json[SERVER_JSON]["id_pin"].asString();	
	return true;
}

/* */
void CoreClass::detectStable(d_type w){
	static d_type weight_temp = 0;
	static unsigned char stable_num = 0;
	static bool isStable = false;
	if(abs(w) > Scale.getStableStep()){
		if (weight_temp == w) {
			if (stable_num <= STABLE_NUM_MAX){
				if (stable_num == STABLE_NUM_MAX) {
					if (!isStable){
						saveEvent("weight", String(w)+"_kg");
						isStable = true;
					}
					return;
				}
				stable_num++;
			}
		} else { 
			stable_num = 0;
			isStable = false;
		}
		weight_temp = w;
	}
}

void powerOff(){
	Scale.power_down(); /// Выключаем ацп
	digitalWrite(EN_NCP, LOW); /// Выключаем стабилизатор
	ESP.reset();
}






