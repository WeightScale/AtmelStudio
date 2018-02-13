#include <Arduino.h>
#include <WiFiClient.h>
#include <WiFiServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266httpUpdate.h>
#include <WiFiUdp.h>

extern "C" uint32_t _SPIFFS_start;
extern "C" uint32_t _SPIFFS_end;

#include "StreamString.h"
#include "Core.h"
#include "HttpUpdater.h"
#include "tools.h"
#include "Version.h"


HttpUpdaterClass httpUpdater;
String updaterError;
int command;

static const char serverIndex[] PROGMEM = R"(<html><body><form method='POST' action='' enctype='multipart/form-data'>
											<input type='file' name='update'>
											<input type='submit' value='Update'>
											</form></body></html>)";

HttpUpdaterClass::HttpUpdaterClass(){
	_server = NULL;
	_username = NULL;
	_password = NULL;
	_authenticated = false;
}

void HttpUpdaterClass::setup(BrowserServerClass *server, const char * path, const char * username, const char * password){
	_server = server;
	_username = (char *)username;
	_password = (char *)password;

	// handler for the /update form page						
	_server->on(path, HTTP_GET, handleUpdatePage);						/* Обновление локально */

	// handler for the /update form POST (once file upload finishes)
	_server->on(path, HTTP_POST, handleEndUpdate, handleStartUpdate);	/* Процесс обновления локально */
	
	_server->on("/hu", HTTP_GET, handleHttpStartUpdate);				/* Обновление чере интернет address/hu?host=sdb.net.ua/update.php */
}

void setUpdaterError(){	
	StreamString str;
	Update.printError(str);
	updaterError = str.c_str();
}

void handleUpdatePage(){
	if(!httpUpdater.getServer()->authenticate(httpUpdater.getUserName(), httpUpdater.getPassword()))
		return httpUpdater.getServer()->requestAuthentication();
	httpUpdater.setAuthenticated(true);
	httpUpdater.getServer()->send_P(200, PSTR(TEXT_HTML), serverIndex);		
}

void handleStartUpdate(){
	digitalWrite(LED, LOW);
	HTTPUpload& upload = httpUpdater.getServer()->upload();
	
	size_t size;
	if(upload.status == UPLOAD_FILE_START){
		updaterError = String();
		
		if(!httpUpdater.getAuthenticated()){
			return;
		}
		
		if(upload.filename.indexOf("spiffs.bin",0) != -1) {
			command = U_SPIFFS;
			size = ((size_t) &_SPIFFS_end - (size_t) &_SPIFFS_start);
		} else if(upload.filename.indexOf("ino.bin",0) != -1) {
			command = U_FLASH;
			size = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
		}else{
			updaterError = "Не верный фаил";
			return;
		}
		WiFiUDP::stopAll();
		if(!Update.begin(size, command)){//start with max available size
			setUpdaterError();
		}
	} else if(httpUpdater.getAuthenticated() && upload.status == UPLOAD_FILE_WRITE && !updaterError.length()){
		if(Update.write(upload.buf, upload.currentSize) != upload.currentSize){
			setUpdaterError();
		}
	} else if(httpUpdater.getAuthenticated() && upload.status == UPLOAD_FILE_END && !updaterError.length()){
		if(!Update.end(true)){ //true to set the size to the current progress
			setUpdaterError();
		}
	} else if(httpUpdater.getAuthenticated() && upload.status == UPLOAD_FILE_ABORTED){
		Update.end();
	}
	delay(0);	
}

void handleEndUpdate(){
	if (updaterError && updaterError[0] != 0x00) {
		httpUpdater.getServer()->send(200, F(TEXT_HTML), String(F("Update error: ")) + updaterError);
	} else {
		if (command == U_SPIFFS){
			delay(2000);
			CORE.saveSettings();
			Scale.saveDate();
			handleFileRead("/");
			return;
		}
		httpUpdater.getServer()->client().setNoDelay(true);
		httpUpdater.getServer()->send_P(200, PSTR(TEXT_HTML), successResponse);
		delay(100);
		httpUpdater.getServer()->client().stop();
		ESP.restart();
	}
}

void handleHttpStartUpdate(){										/* Обновление чере интернет address/hu?host=sdb.net.ua/update.php */
	if(!httpUpdater.getServer()->authenticate(httpUpdater.getUserName(), httpUpdater.getPassword()))
		return httpUpdater.getServer()->requestAuthentication();
	if(httpUpdater.getServer()->hasArg("host")){
		String host = httpUpdater.getServer()->arg("host");
		//_server->send(200, "text/plain", host);
		ESPhttpUpdate.rebootOnUpdate(false);
		digitalWrite(LED, LOW);
		String url = String("http://");
		url += host;
		t_httpUpdate_return ret = ESPhttpUpdate.updateSpiffs(url, SPIFFS_VERSION);
		if (ret == HTTP_UPDATE_OK){
			CORE.saveSettings();
			Scale.saveDate();
			ret = ESPhttpUpdate.update(url, SKETCH_VERSION);
		}
		switch(ret) {
			case HTTP_UPDATE_FAILED:
				httpUpdater.getServer()->send(404, "text/plain", ESPhttpUpdate.getLastErrorString());
			break;
			case HTTP_UPDATE_NO_UPDATES:
				httpUpdater.getServer()->send(304, "text/plain", "Обновление не требуется");
			break;
			case HTTP_UPDATE_OK:
				httpUpdater.getServer()->client().stop();
				ESP.restart();
			break;
		}
		
	}
	digitalWrite(LED, HIGH);		
};
	