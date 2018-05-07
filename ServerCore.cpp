/* 
* ServerCoreClass.cpp
*
* Created: 20.04.2018 13:40:19
* Author: Kostya
*/
#include "ServerCore.h"

ServerCoreClass ServerCore(80);


ServerCoreClass::ServerCoreClass(uint16_t port) : AsyncWebServer(port){
	SPIFFS.begin();	
}

ServerCoreClass::~ServerCoreClass(){} 
	
ServerCoreClass::init(){
	_downloadHTTPAuth();
}

bool ServerCoreClass::_downloadHTTPAuth() {
	_httpAuth.wwwUsername = "sa";
	_httpAuth.wwwPassword = "343434";
	File configFile = SPIFFS.open(AUTH_FILE, "r");
	if (!configFile) {
		configFile.close();
		return false;
	}
	size_t size = configFile.size();

	std::unique_ptr<char[]> buf(new char[size]);
	
	configFile.readBytes(buf.get(), size);
	configFile.close();
	DynamicJsonBuffer jsonBuffer(256);
	JsonObject& json = jsonBuffer.parseObject(buf.get());

	if (!json.success()) {
		return false;
	}
	_httpAuth.wwwUsername = json["user"].as<String>();
	_httpAuth.wwwPassword = json["pass"].as<String>();
	return true;
}

bool BrowserServerClass::_saveHTTPAuth() {
	
	DynamicJsonBuffer jsonBuffer(256);
	//StaticJsonBuffer<256> jsonBuffer;
	JsonObject& json = jsonBuffer.createObject();
	json["user"] = _httpAuth.wwwUsername;
	json["pass"] = _httpAuth.wwwPassword;

	//TODO add AP data to html
	File configFile = SPIFFS.open(AUTH_FILE, "w");
	if (!configFile) {
		configFile.close();
		return false;
	}

	json.printTo(configFile);
	configFile.flush();
	configFile.close();
	return true;
}
