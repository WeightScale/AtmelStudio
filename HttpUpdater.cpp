#include <Arduino.h>
#include <WiFiClient.h>
#include <WiFiServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <WiFiUdp.h>
#include "StreamString.h"
#include "HttpUpdater.h"

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
		if (Update.hasError()) {
			_server->send(200, F("text/html"), String(F("Update error: ")) + _updaterError);
		} else {
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

		if(upload.status == UPLOAD_FILE_START){
			_updaterError = String();			

			_authenticated = (_username == NULL || _password == NULL || _server->authenticate(_username, _password));
			if(!_authenticated){				
				return;
			}

			WiFiUDP::stopAll();			
			uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
			if(!Update.begin(maxSketchSpace)){//start with max available size
				_setUpdaterError();
			}
		} else if(_authenticated && upload.status == UPLOAD_FILE_WRITE && !_updaterError.length()){
			//if (_serial_output) Serial.printf(".");
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
}

void HttpUpdaterClass::_setUpdaterError(){	
	StreamString str;
	Update.printError(str);
	_updaterError = str.c_str();
}

