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
	Rtc.Begin();
	_downloadSettings();
	_callibratedBaterry();	
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
	
	bool flag = WiFi.status() == WL_CONNECTED?eventToServer(date, event, value):false;
	
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
String CoreClass::getIp() {
	WiFiClient client ;
	String ip = "";
	if (client.connect("api.ipify.org", 80)) {
		client.println("GET / HTTP/1.0");
		client.println("Host: api.ipify.org");
		client.println();
		delay(50);
		while(client.available()){
			String ip = client.readStringUntil('\n');
		}		
	}
	client.stop();	
	return ip;
}*/

String CoreClass::getIp(){	
	HTTPClient http;	
	http.begin("http://sdb.net.ua/ip.php");
	http.setTimeout(_settings.timeout);	
	int httpCode = http.GET();
	String ip = http.getString();
	http.end();	
	if(httpCode == HTTP_CODE_OK){		
		return ip;
	}	
	return String(httpCode);
}

/* */	
bool CoreClass::eventToServer(const String& date, const String& type, const String& value){
	HTTPClient http;
	String message = "http://";
	message += _settings.hostUrl.c_str();
	String hash = getHash(_settings.hostPin.c_str(), date, type, value);	
	message += "/scales.php?hash=" + hash;
	http.begin(message);
	http.setTimeout(_settings.timeout);
	int httpCode = http.GET();
	http.end();
	if(httpCode == HTTP_CODE_OK) {
		return true;
	}
	return false;
}

void CoreClass::saveValueSettingsHttp(const char * text) {	
		String message = " ";
		if (browserServer.hasArg("ssid")){
			_settings.autoIp = false;
			if (browserServer.hasArg("auto"))
				_settings.autoIp = true;
			else
				_settings.autoIp = false;				 			
			_settings.scaleLanIp = browserServer.urldecode(browserServer.arg("lan_ip"));			
			_settings.scaleGateway = browserServer.urldecode(browserServer.arg("gateway"));
			_settings.scaleSubnet = browserServer.urldecode(browserServer.arg("subnet"));
			_settings.scaleWlanSSID = browserServer.urldecode(browserServer.arg("ssid"));
			_settings.scaleWlanKey = browserServer.urldecode(browserServer.arg("key"));	
			goto save;
		}
		if(browserServer.hasArg("data")){
			DateTimeClass DateTime(browserServer.arg("data"));
			Rtc.SetDateTime(DateTime.toRtcDateTime());
			String message = getDateTime();
			browserServer.send(200, TEXT_HTML, message);
			return;	
		}
		if (browserServer.hasArg("host")){
			_settings.hostUrl = browserServer.urldecode(browserServer.arg("host"));
			_settings.hostPin = browserServer.urldecode(browserServer.arg("pin"));
			goto save;	
		}
		if (browserServer.hasArg("name_admin")){
			_settings.scaleName = browserServer.urldecode(browserServer.arg("name_admin"));
			_settings.scalePass = browserServer.urldecode(browserServer.arg("pass_admin"));
			goto save;
		}		
		save:
		if (saveSettings()){
			return browserServer.send(200, TEXT_HTML, text);
		}
		browserServer.send(400, TEXT_HTML, text);
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
	_charge = getADC(times);
	_charge = constrain(_charge, MIN_CHG, _settings.bat_max);
	_charge = map(_charge, MIN_CHG, _settings.bat_max, 0, 100);
	return _charge;	
}

bool CoreClass::saveSettings() {	
	File serverFile = SPIFFS.open(SETTINGS_FILE, "w+");
	if (!serverFile) {
		serverFile.close();
		return false;
	}
	
	DynamicJsonBuffer jsonBuffer;
	JsonObject& json = jsonBuffer.createObject();

	if (!json.containsKey(SCALE_JSON)) {
		JsonObject& scale = json.createNestedObject(SCALE_JSON);
	}
	
	json[SCALE_JSON]["id_name_admin"] = _settings.scaleName;
	json[SCALE_JSON]["id_pass_admin"] = _settings.scalePass;
	json[SCALE_JSON]["id_auto"] = _settings.autoIp;
	json[SCALE_JSON]["id_lan_ip"] = _settings.scaleLanIp;
	json[SCALE_JSON]["id_gateway"] = _settings.scaleGateway;
	json[SCALE_JSON]["id_subnet"] = _settings.scaleSubnet;
	json[SCALE_JSON]["id_ssid"] = _settings.scaleWlanSSID;
	json[SCALE_JSON]["id_key"] = _settings.scaleWlanKey;
	json[SCALE_JSON]["bat_max"] = _settings.bat_max;	
	
	if (!json.containsKey(SERVER_JSON)) {
		JsonObject& server = json.createNestedObject(SERVER_JSON);
	}
	
	json[SERVER_JSON]["id_host"] = _settings.hostUrl;
	json[SERVER_JSON]["id_pin"] = _settings.hostPin;
	json[SERVER_JSON]["timeout"] = _settings.timeout;

	json.printTo(serverFile);
	serverFile.flush();
	serverFile.close();
	return true;
}

bool CoreClass::_downloadSettings() {
	_settings.scaleName = "admin";
	_settings.scalePass = "1234";
	_settings.autoIp = true;
	_settings.scaleLanIp = "192.168.1.100";
	_settings.scaleGateway = "192.168.1.1";
	_settings.scaleSubnet = "255.255.255.0";
	_settings.hostUrl = HOST_URL;
	_settings.timeout = TIMEOUT_HTTP;
	_settings.bat_max = MIN_CHG+1;
	File serverFile;
	if (SPIFFS.exists(SETTINGS_FILE)){
		serverFile = SPIFFS.open(SETTINGS_FILE, "r");	
	}else{
		serverFile = SPIFFS.open(SETTINGS_FILE, "w+");
	}
	
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
	if (json.containsKey(SCALE_JSON)){
		_settings.scaleName = json[SCALE_JSON]["id_name_admin"].as<String>();
		_settings.scalePass = json[SCALE_JSON]["id_pass_admin"].as<String>();
		_settings.autoIp = json[SCALE_JSON]["id_auto"];
		_settings.scaleLanIp = json[SCALE_JSON]["id_lan_ip"].as<String>();
		_settings.scaleGateway = json[SCALE_JSON]["id_gateway"].as<String>();
		_settings.scaleSubnet = json[SCALE_JSON]["id_subnet"].as<String>();
		_settings.scaleWlanSSID = json[SCALE_JSON]["id_ssid"].as<String>();
		_settings.scaleWlanKey = json[SCALE_JSON]["id_key"].as<String>();
		_settings.bat_max = json[SCALE_JSON]["bat_max"];	
	}
	if (json.containsKey(SERVER_JSON)){
		_settings.hostUrl = json[SERVER_JSON]["id_host"].as<String>();
		_settings.hostPin = json[SERVER_JSON]["id_pin"].as<String>();	
		_settings.timeout = json[SERVER_JSON]["timeout"];	
	}
		
	_settings.bat_max = constrain(_settings.bat_max, MIN_CHG+1, 1024);
	return true;
}

/* */
void CoreClass::detectStable(float w){
	static float weight_temp;
	static unsigned char stable_num;
	static bool isStable;
	if(abs(w) > Scale.getStableStep()){
		if (weight_temp == w) {
			//if (stable_num <= STABLE_NUM_MAX){
				if (stable_num > STABLE_NUM_MAX) {
					if (!isStable){
						saveEvent("weight", String(w)+"_kg");
						isStable = true;
					}
					return;
				}
				stable_num++;
			//}
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

void reconnectWifi(){
	browserServer.client().stop();
	connectWifi();	
}

int CoreClass::getADC(byte times){
	long sum = 0;
	for (byte i = 0; i < times; i++) {
		sum += analogRead(A0);
	}
	return times == 0?sum :sum / times;	
}

void CoreClass::_callibratedBaterry(){
	int charge = getADC();
	
	charge = constrain(charge, MIN_CHG, 1024);
	if (_settings.bat_max < MIN_CHG){
		_settings.bat_max = MIN_CHG;
	}
	if (_settings.bat_max < charge){
		_settings.bat_max = charge;	
		saveSettings();
	}
}






