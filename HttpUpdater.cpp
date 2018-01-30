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
	_server->on(path, HTTP_GET, [&](){
		if(_username != NULL && _password != NULL && !_server->authenticate(_username, _password))
			return _server->requestAuthentication();
		_server->send_P(200, PSTR("text/html"), serverIndex);
	});

	// handler for the /update form POST (once file upload finishes)
	_server->on(path, HTTP_POST, [&](){
		if(!_authenticated)
			return _server->requestAuthentication();
		if (_updaterError && _updaterError[0] != 0x00) {
			_server->send(200, F("text/html"), String(F("Update error: ")) + _updaterError);
		} else {	
			if (_command == U_SPIFFS){	
				delay(2000);
				CORE.saveSettings();
				Scale.saveDate();		
				handleFileRead("/");
				return;
			}		
			_server->client().setNoDelay(true);			
			_server->send_P(200, PSTR("text/html"), successResponse);
			delay(100);
			_server->client().stop();
			ESP.restart();
		}
		},[&](){
		// handler for the file upload, get's the sketch bytes, and writes
		// them through the Update object
		digitalWrite(LED, LOW);
		HTTPUpload& upload = _server->upload();
		
		size_t size;
		if(upload.status == UPLOAD_FILE_START){
			_updaterError = String();			

			_authenticated = (_username == NULL || _password == NULL || _server->authenticate(_username, _password));
			if(!_authenticated){				
				return;
			}	
						
			if(upload.filename.indexOf("spiffs.bin",0) != -1) {
				_command = U_SPIFFS;
				size = ((size_t) &_SPIFFS_end - (size_t) &_SPIFFS_start);				
			} else if(upload.filename.indexOf("ino.bin",0) != -1) {
				_command = U_FLASH;
				size = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
			}else{
				_updaterError = "Не верный фаил";
				return;
			}
			WiFiUDP::stopAll();
			if(!Update.begin(size, _command)){//start with max available size
				_setUpdaterError();
			}
		} else if(_authenticated && upload.status == UPLOAD_FILE_WRITE && !_updaterError.length()){
			if(Update.write(upload.buf, upload.currentSize) != upload.currentSize){
				_setUpdaterError();
			}
		} else if(_authenticated && upload.status == UPLOAD_FILE_END && !_updaterError.length()){
			if(!Update.end(true)){ //true to set the size to the current progress				
				_setUpdaterError();
			}			
		} else if(_authenticated && upload.status == UPLOAD_FILE_ABORTED){
			Update.end();			
		}
		delay(0);
	});
	
	_server->on("/hu", HTTP_GET, [&]() {								/* Обновление чере интернет */
		if (!_server->checkAdminAuth())
		return _server->requestAuthentication();
		ESPhttpUpdate.rebootOnUpdate(false);
		digitalWrite(LED, LOW);
		_server->send(404, "text/plain", "Проверяем");
		t_httpUpdate_return ret = ESPhttpUpdate.updateSpiffs("http://sdb.net.ua/update.php", SPIFFS_VERSION);
		if (ret == HTTP_UPDATE_OK){
			ret = ESPhttpUpdate.update("http://sdb.net.ua/update.php", SKETCH_VERSION);
		}
		switch(ret) {
			case HTTP_UPDATE_FAILED:
			_server->send(404, "text/plain", ESPhttpUpdate.getLastErrorString());
			break;
			case HTTP_UPDATE_NO_UPDATES:
			_server->send(304, "text/plain", "Обновление не требуется");
			break;
			case HTTP_UPDATE_OK:
			_server->send(200, "text/plain", "Обновление успешно!");
			break;
		}
		digitalWrite(LED, HIGH);
	});
}

void HttpUpdaterClass::_setUpdaterError(){	
	StreamString str;
	Update.printError(str);
	_updaterError = str.c_str();
}

