#include <ESP8266WebServer.h>
#include <WiFiClient.h>
#include <StreamString.h>
#include <ArduinoJson.h>
#include "BrowserServer.h"
#include "handleHttp.h"
#include "Core.h"
#include "Version.h"
#include "HttpUpdater.h"

static const char netIndex[]= /*PROGMEM =*/ R"(	<html><meta name='viewport' content='width=device-width, initial-scale=1, maximum-scale=1'/>
												<body><form method='POST'>
												<input type='checkbox' name='auto'><br/>												
												<input name='ssid'><br/>
												<input type='password' name='key'><br/>
												<input type='submit' value='СОХРАНИТЬ'>
												</form></body></html>)";
/* */
//ESP8266HTTPUpdateServer httpUpdater;
/* Soft AP network parameters */
IPAddress apIP(192,168,4,1);
IPAddress netMsk(255, 255, 255, 0);

IPAddress lanIp;			// Надо сделать настройки ip адреса
IPAddress gateway;

BrowserServerClass browserServer(80);
DNSServer dnsServer;
//holds the current upload
File fsUploadFile;

/* hostname for mDNS. Should work at least on windows. Try http://esp8266.local */


BrowserServerClass::BrowserServerClass(uint16_t port) : ESP8266WebServer(port) {}

BrowserServerClass::~BrowserServerClass(){}
	
void BrowserServerClass::begin() {
	
	/* Setup the DNS server redirecting all the domains to the apIP */
	dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
	dnsServer.start(DNS_PORT, "*", apIP);
		
	ESP8266WebServer::begin(); // Web server start
	_downloadHTTPAuth();	
	init();
}

void BrowserServerClass::init(){											
	on("/",[&](){												/* Главная страница. */
		handleFileRead(uri());
		taskPower.resume();
	});
	on("/rc", reconnectWifi);									/* Пересоединиться по WiFi. */
	on("/sn",HTTP_GET,handleAccessPoint);						/* Установить Настройки точки доступа */
	on("/sn",HTTP_POST, handleSetAccessPoint);					/* Установить Настройки точки доступа */
	on("/settings.html", handleSettingsHtml);					/* Открыть страницу настроек или сохранить значения. */	
	on("/settings.json", handleFileReadAuth);
	on("/sv", handleScaleProp);									/* Получить значения. */	
	//list directory
	on("/list", HTTP_GET, handleFileList);
	//load editor
	on("/editor.html", HTTP_GET, [&](){
		if (!checkAdminAuth())
			return requestAuthentication();
		if(!handleFileRead("/editor.html")) 
			send(404, "text/plain", "FileNotFound");
	});
	//create file
	on("/edit", HTTP_PUT, handleFileCreate);
	//delete file
	on("/edit", HTTP_DELETE, handleFileDelete);
	on("/edit", HTTP_POST, [&](){ 
		if (!checkAdminAuth())
			return requestAuthentication();
		send(200, "text/plain", ""); }, handleFileUpload);
	
	on("/admin.html", handleAuthConfiguration);
	on("/secret.json",handleFileReadAdmin);
	onNotFound([&](){
		if(isValidType(uri())){
			if(!handleFileRead(uri()))
				return send(404, "text/plain", "FileNotFound");	
		}else{			
			if(!handleFileReadAuth())
				return send(404, "text/plain", "FileNotFound");		
		}		
	});
	//on("/generate_204", [this](){if (!handleFileRead("/index.html"))	this->send(404, "text/plain", "FileNotFound");});  //Android captive portal. Maybe not needed. Might be handled by notFound handler.
	//on("/fwlink", [this](){if (!handleFileRead("/index.html"))	this->send(404, "text/plain", "FileNotFound");});  //Microsoft captive portal. Maybe not needed. Might be handled by notFound handler.	
	
	const char * headerkeys[] = {"User-Agent","Cookie"/*,"x-SETNET"*/} ;
	size_t headerkeyssize = sizeof(headerkeys)/sizeof(char*);
	//ask server to track these headers
	collectHeaders(headerkeys, headerkeyssize );		
}

/*
void send_update_firmware_values_html() {
	if (!browserServer.checkAdminAuth())
		return browserServer.requestAuthentication();
	String values = "";
	uint32_t maxSketchSpace = (ESP.getSketchSize() - 0x1000) & 0xFFFFF000;
	//bool updateOK = Update.begin(maxSketchSpace);
	bool updateOK = maxSketchSpace < ESP.getFreeSketchSpace();
	StreamString result;
	Update.printError(result);	
	values += "remupd|" + (String)((updateOK) ? "OK" : "ERROR") + "|div\n";

	if (Update.hasError()) {
		result.trim();
		values += "remupdResult|" + result + "|div\n";
	} else {
		values += "remupdResult||div\n";
	}

	browserServer.send(200, "text/plain", values);
}*/

void BrowserServerClass::send_wwwauth_configuration_html() {
	if (args() > 0){  // Save Settings	
		_httpAuth.auth = false;
		//String temp = "";
		if (hasArg("wwwuser")){
			_httpAuth.wwwUsername = arg("wwwuser");
			_httpAuth.wwwPassword = arg("wwwpass");
			if (hasArg("wwwauth")){
				_httpAuth.auth = true;
			}	
		}		
		_saveHTTPAuth();
	}
	handleFileRead(uri());
}

bool BrowserServerClass::_saveHTTPAuth() {
	
	DynamicJsonBuffer jsonBuffer(256);
	//StaticJsonBuffer<256> jsonBuffer;
	JsonObject& json = jsonBuffer.createObject();
	json["auth"] = _httpAuth.auth;
	json["user"] = _httpAuth.wwwUsername;
	json["pass"] = _httpAuth.wwwPassword;

	//TODO add AP data to html
	File configFile = SPIFFS.open(SECRET_FILE, "w");
	if (!configFile) {
		configFile.close();
		return false;
	}

	json.printTo(configFile);
	configFile.flush();
	configFile.close();
	return true;
}

bool BrowserServerClass::_downloadHTTPAuth() {
	_httpAuth.auth = false;
	_httpAuth.wwwUsername = "sa";
	_httpAuth.wwwPassword = "343434";
	File configFile = SPIFFS.open(SECRET_FILE, "r");
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
	_httpAuth.auth = json["auth"];
	_httpAuth.wwwUsername = json["user"].as<String>();
	_httpAuth.wwwPassword = json["pass"].as<String>();
	return true;
}

bool BrowserServerClass::checkAdminAuth() {
	if (!_httpAuth.auth) {
		return true;
	} else {
		return authenticate(_httpAuth.wwwUsername.c_str(), _httpAuth.wwwPassword.c_str());
	}
}

/*
void BrowserServerClass::restart_esp() {
	String message = "<meta name='viewport' content='width=device-width, initial-scale=1, maximum-scale=1'/>";
	message += "<meta http-equiv='refresh' content='10; URL=/admin.html'>Please Wait....Configuring and Restarting.";
	send(200, "text/html", message);
	SPIFFS.end(); // SPIFFS.end();
	delay(1000);
	ESP.restart();
}*/

// convert a single hex digit character to its integer value (from https://code.google.com/p/avr-netino/)
unsigned char BrowserServerClass::h2int(char c) {
	if (c >= '0' && c <= '9') {
		return((unsigned char)c - '0');
	}
	if (c >= 'a' && c <= 'f') {
		return((unsigned char)c - 'a' + 10);
	}
	if (c >= 'A' && c <= 'F') {
		return((unsigned char)c - 'A' + 10);
	}
	return(0);
}

String BrowserServerClass::urldecode(String input){ // (based on https://code.google.com/p/avr-netino/)
	char c;
	String ret = "";

	for (byte t = 0; t < input.length(); t++) {
		c = input[t];
		if (c == '+') c = ' ';
		if (c == '%') {
			t++;
			c = input[t];
			t++;
			c = (h2int(c) << 4) | h2int(input[t]);
		}

		ret.concat(c);
	}
	return ret;
}

/*
void setUpdateMD5() {
	if (!browserServer.checkAdminAuth())
		return browserServer.requestAuthentication();
	String _browserMD5 = "";
	if (browserServer.args() > 0){  // Read hash
		for (uint8_t i = 0; i < browserServer.args(); i++) {			
			if (browserServer.argName(i) == "md5") {
				_browserMD5 = browserServer.urldecode(browserServer.arg(i));
				Update.setMD5(_browserMD5.c_str());
				continue;
			}if (browserServer.argName(i) == "size") {
				//_updateSize = browserServer.arg(i).toInt();				
				continue;
			}
		}
		browserServer.send(200, TEXT_HTML, "OK --> MD5: " + _browserMD5);
	}
}*/



String BrowserServerClass::getContentType(String filename){
	if(hasArg("download")) return "application/octet-stream";
	else if(filename.endsWith(".htm")) return TEXT_HTML;
	else if(filename.endsWith(".html")) return TEXT_HTML;
	else if(filename.endsWith(".css")) return "text/css";
	else if(filename.endsWith(".js")) return "application/javascript";
	else if (filename.endsWith(".json")) return "application/json";
	else if(filename.endsWith(".png")) return "image/png";
	else if(filename.endsWith(".gif")) return "image/gif";
	else if(filename.endsWith(".jpg")) return "image/jpeg";
	else if(filename.endsWith(".ico")) return "image/x-icon";
	else if(filename.endsWith(".xml")) return "text/xml";
	else if(filename.endsWith(".pdf")) return "application/x-pdf";
	else if(filename.endsWith(".zip")) return "application/x-zip";
	else if(filename.endsWith(".gz")) return "application/x-gzip";
	return "text/plain";
}

bool BrowserServerClass::isValidType(String filename){
	if(filename.endsWith(".css")) return true;
	else if(filename.endsWith(".js")) return true;
	else if(filename.endsWith(".png")) return true;
	else if(filename.endsWith(".ico")) return true;
	else if(filename.endsWith(".json")) return true;
	else if(filename.endsWith(".html")) return true;
	return false;	
}

bool BrowserServerClass::isAuthentified(){
	if (!authenticate(CORE.getNameAdmin().c_str(), CORE.getPassAdmin().c_str())){
		if (!checkAdminAuth()){
			return false;
		}
	}
	return true;
}

bool handleFileReadAdmin(){
	if (!browserServer.checkAdminAuth()){
		browserServer.requestAuthentication();
		return false;
	}
	return handleFileRead(browserServer.uri());
}

bool handleFileReadAuth(){
	if (!browserServer.isAuthentified()){
		browserServer.requestAuthentication();
		return false;
	}
	return handleFileRead(browserServer.uri());
}

bool handleFileRead(String path){
	if(path.endsWith("/")) 
		path += "index.html";
	String contentType = browserServer.getContentType(path);
	String pathWithGz = path + ".gz";
	if(SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)){
		if(SPIFFS.exists(pathWithGz))
			path += ".gz";
		File file = SPIFFS.open(path, "r");
		size_t sent = browserServer.streamFile(file, contentType);
		file.close();
		return true;
	}
	return false;
}

void handleFileUpload(){
	if(browserServer.uri() != "/edit") 
		return;
	HTTPUpload& up = browserServer.upload();
	if(up.status == UPLOAD_FILE_START){
		String filename = up.filename;
		if(!filename.startsWith("/")) 
			filename = "/"+filename;
		fsUploadFile = SPIFFS.open(filename, "w");
		filename = String();
	} else if(up.status == UPLOAD_FILE_WRITE){
		if(fsUploadFile)
			fsUploadFile.write(up.buf, up.currentSize);
	} else if(up.status == UPLOAD_FILE_END){
		if(fsUploadFile)
		fsUploadFile.close();		
	}
}

void handleFileDelete(){
	if (!browserServer.checkAdminAuth())
		return browserServer.requestAuthentication();
	if(browserServer.args() == 0) 
		return browserServer.send(500, "text/plain", "BAD ARGS");
	String path = browserServer.arg(0);
	if(path == "/")
		return browserServer.send(500, "text/plain", "BAD PATH");
	if(!SPIFFS.exists(path))
		return browserServer.send(404, "text/plain", "FileNotFound");
	SPIFFS.remove(path);
		browserServer.send(200, "text/plain", "");
	path = String();
}

void handleFileCreate(){
	if (!browserServer.checkAdminAuth())
		return browserServer.requestAuthentication();
	if(browserServer.args() == 0)
		return browserServer.send(500, "text/plain", "BAD ARGS");
	String path = browserServer.arg(0);
	if(path == "/")
		return browserServer.send(500, "text/plain", "BAD PATH");
	if(SPIFFS.exists(path))
		return browserServer.send(500, "text/plain", "FILE EXISTS");
	File file = SPIFFS.open(path, "w");
	if(file)
		file.close();
	else
		return browserServer.send(500, "text/plain", "CREATE FAILED");
	browserServer.send(200, "text/plain", "");
	path = String();
}

void handleFileList() {	
	if (!browserServer.checkAdminAuth())
		return browserServer.requestAuthentication();
	if(!browserServer.hasArg("dir")) {
		browserServer.send(500, "text/plain", "BAD ARGS"); 
		return;
	}
	
	String path = browserServer.arg("dir");
	Dir dir = SPIFFS.openDir(path);
	path = String();

	String output = "[";
	while(dir.next()){
		File entry = dir.openFile("r");
		if (output != "[") output += ',';
		bool isDir = false;
		output += "{\"type\":\"";
			output += (isDir)?"dir":"file";
			output += "\",\"name\":\"";
			output += String(entry.name()).substring(1);
		output += "\"}";
		entry.close();
	}
	
	output += "]";
	browserServer.send(200, "text/json", output);
}

void handleAccessPoint(){
	if (!browserServer.checkAdminAuth())
		return browserServer.requestAuthentication();
	browserServer.send(200, TEXT_HTML, netIndex);	
}

void handleSetAccessPoint(){
	CORE.saveValueSettingsHttp(successResponse);
	delay(100);
	browserServer.client().stop();
	ESP.restart();	
}

void handleAuthConfiguration(){
	if (!browserServer.checkAdminAuth())
		return browserServer.requestAuthentication();
	browserServer.send_wwwauth_configuration_html();
	taskPower.pause();	
}
