 
#include <ESP8266WebServer.h>
#include <WiFiClient.h>
#include <StreamString.h>
#include <ESP8266httpUpdate.h>
//#include <ArduinoOTA.h>
#include <ArduinoJson.h>
#include "BrowserServer.h"
#include "handleHttp.h"
#include "Scales.h"
#include "Version.h"

/* */
//ESP8266HTTPUpdateServer httpUpdater;
/* Soft AP network parameters */
//IPAddress apIP(192, 168, 4, 1);
IPAddress apIP(192,168,4,1);
IPAddress netMsk(255, 255, 255, 0);

IPAddress lanIp;			// Надо сделать настройки ip адреса
IPAddress gateway;

BrowserServerClass browserServer(80);
DNSServer dnsServer;
//holds the current upload
File fsUploadFile;

/** Should I connect to WLAN asap? */
boolean connect;
/* Set these to your desired softAP credentials. They are not configurable at runtime */
const char *softAP_ssid = "SCALES";
const char *softAP_password = "12345678";

//const char* super_user_login = "su";
//const char* super_user_password = "1234";

/* hostname for mDNS. Should work at least on windows. Try http://esp8266.local */
const char *myHostname = "scales";

BrowserServerClass::BrowserServerClass(uint16_t port) : ESP8266WebServer(port) {}

BrowserServerClass::~BrowserServerClass(){}
	
void BrowserServerClass::begin() {
	
	/* Setup the DNS server redirecting all the domains to the apIP */
	dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
	dnsServer.start(DNS_PORT, "*", apIP);
		
	ESP8266WebServer::begin(); // Web server start
	loadHTTPAuth();	
	init();
}

int getCountsOfDigits(long number) {
	int count = 0;
	while (number != 0) {
		count++;
		number /= 10;
	}
	return count;
}

void BrowserServerClass::init(){	
	on("/weight", [this](){
		char buffer[10];		
		SCALES.setWeight(SCALES.get_units());
		SCALES.formatValue(SCALES.getWeight(), buffer);							
		this->send(200, "text/plain", String("{\"w\":\""+String(buffer)+"\",\"c\":"+String(SCALES.getCharge())+"}"));
		SCALES.detectStable();
		taskPower.updateCache();		
	});
	on("/tape", [this](){
		SCALES.tare();
		this->send(204, "text/html", "");
	});
	on("/",[this](){
		if (!handleFileRead("/index.html"))	
			this->send(404, "text/plain", "FileNotFound");
		taskPower.resume();
	});
	on("/settings.html", [this]() {
		if (!isAuthentified())
			return this->requestAuthentication();
		handlePropSave();
	});
	on("/scale/values", handleScaleProp);
	on("/calibr.html", [this]() {
		if (!isAuthentified())
			return this->requestAuthentication();
		handleCalibrSave();
		taskPower.pause();
	});
	on("/seal", [this](){
		if (SCALES.saveDate()){
			SCALES.updateSettings();
			this->send(200, "text/html", "");
		}
	});
	on("/events.html", [this](){
		handleFileRead("/events.html");});
	//list directory
	on("/list", HTTP_GET, [this](){
		if (!this->checkAuth())
			return this->requestAuthentication(); 
		handleFileList();});
	//load editor
	on("/editor.html", HTTP_GET, [this](){
		if (!this->checkAuth())
			return this->requestAuthentication();
		if(!handleFileRead("/editor.html")) 
			send(404, "text/plain", "FileNotFound");});
	//create file
	on("/edit", HTTP_PUT, [this](){
		if (!this->checkAuth())
			return this->requestAuthentication();
		handleFileCreate();});
	//delete file
	on("/edit", HTTP_DELETE, [this](){
		if (!this->checkAuth())
			return this->requestAuthentication();
		handleFileDelete();});
	on("/edit", HTTP_POST, [this](){ 
		if (!this->checkAuth())
			return this->requestAuthentication();
		this->send(200, "text/plain", ""); }, handleFileUpload);
	
	onNotFound([this](){
		if(isValidType(uri())){
			if(!handleFileRead(uri()))
				return send(404, "text/plain", "FileNotFound");	
		}else{
			if (!this->checkAuth())
				return this->requestAuthentication();
			if(!handleFileRead(uri()))
				return send(404, "text/plain", "FileNotFound");		
		}		
	});
	on("/httpupdate", HTTP_GET, [this]() {
		if (!this->checkAuth())
			return this->requestAuthentication();
		t_httpUpdate_return ret = ESPhttpUpdate.update("sdb.net.ua", 80, "/esp/update.php", "wshx711_v_" + String(SKETCH_VERSION));
		switch(ret) {
			case HTTP_UPDATE_FAILED:
				this->send(404, "text/plain", "Обновление выполнить не удалось");
			break;
			case HTTP_UPDATE_NO_UPDATES:
				this->send(304, "text/plain", "Обновление не требуется");
			break;
			case HTTP_UPDATE_OK:
				this->send(200, "text/plain", "Обновление успешно!");
			break;
		}
	});	
	on("/update", HTTP_GET, [this]() {	
		if (!this->checkAuth())
			return this->requestAuthentication();	
		if (!handleFileRead("/update.html"))
			this->send(404, "text/plain", "FileNotFound");
	});
	on("/update/updatepossible", [this]() {
		if (!this->checkAuth())
			return this->requestAuthentication();
		this->send_update_firmware_values_html();
	});
	on("/setmd5", [this]() {
		if (!this->checkAuth())
			return this->requestAuthentication();
		this->setUpdateMD5();
	});
	on("/update", HTTP_POST, [this](){
		if (!this->checkAuth())
			return this->requestAuthentication();
		this->sendHeader("Connection", "close");
		this->sendHeader("Access-Control-Allow-Origin", "*");
		String message = "<meta name='viewport' content='width=device-width, initial-scale=1, maximum-scale=1'/>";
		message += "<meta http-equiv='refresh' content='15;URL=/admin.html'/>Update correct. Restarting...";
		this->send(200, "text/html", (Update.hasError())?"FAIL": message);
		ESP.restart();
		},[this](){
		HTTPUpload& upload = this->upload();
		if(upload.status == UPLOAD_FILE_START){
			Serial.setDebugOutput(true);
			WiFiUDP::stopAll();
			#if defined SERIAL_DEDUG
				Serial.printf("Update: %s\n", upload.filename.c_str());
			#endif			
			uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
			if(!Update.begin(maxSketchSpace)){//start with max available size
				Update.printError(Serial);
			}
		} else if(upload.status == UPLOAD_FILE_WRITE){
			if(Update.write(upload.buf, upload.currentSize) != upload.currentSize){
				Update.printError(Serial);
			}
		} else if(upload.status == UPLOAD_FILE_END){
			if(Update.end(true)){ //true to set the size to the current progress
				#if defined SERIAL_DEDUG
					Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
				#endif				
			} else {
				Update.printError(Serial);
			}
			Serial.setDebugOutput(false);
		}
		yield();
	});	
	on("/admin.html", [this]() {
		if (!this->checkAuth())
			return this->requestAuthentication();
		this->send_wwwauth_configuration_html();
		taskPower.pause();
	});
	/*on("/admin/restart", [this]() {	
		if (!this->checkAuth())
			return this->requestAuthentication();
		this->restart_esp();});*/	
	/*on("/admin/wwwauth", [this]() {
		if (!this->checkAuth())
			return this->requestAuthentication();
		this->send_wwwauth_configuration_values_html();
	});*/
	on("/secret.json",[this]() {
		if (!this->checkAuth())
			return this->requestAuthentication();
		handleFileRead("/secret.json");
	});	
	on("/settings.json",[this]() {
		if (!isAuthentified())
			return this->requestAuthentication();
		handleFileRead("/settings.json");
	});
	//on("/generate_204", [this](){if (!handleFileRead("/index.html"))	this->send(404, "text/plain", "FileNotFound");});  //Android captive portal. Maybe not needed. Might be handled by notFound handler.
	//on("/fwlink", [this](){if (!handleFileRead("/index.html"))	this->send(404, "text/plain", "FileNotFound");});  //Microsoft captive portal. Maybe not needed. Might be handled by notFound handler.	
	
	const char * headerkeys[] = {"User-Agent","Cookie","x-SETNET"} ;
	size_t headerkeyssize = sizeof(headerkeys)/sizeof(char*);
	//ask server to track these headers
	collectHeaders(headerkeys, headerkeyssize );
	#if defined SERIAL_DEDUG
		Serial.println("HTTP server started");
	#endif	
}

void BrowserServerClass::send_update_firmware_values_html() {
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

	send(200, "text/plain", values);
}

void BrowserServerClass::send_wwwauth_configuration_html() {
	if (args() > 0){  // Save Settings
	
		_httpAuth.auth = false;
		//String temp = "";
		for (uint8_t i = 0; i < args(); i++) {
			if (argName(i) == "wwwuser") {
				_httpAuth.wwwUsername = urldecode(arg(i));
				continue;
			}
			if (argName(i) == "wwwpass") {
				_httpAuth.wwwPassword = urldecode(arg(i));
				continue;
			}
			if (argName(i) == "wwwauth") {
				_httpAuth.auth = true;
				continue;
			}
		}

		saveHTTPAuth();
	}
	handleFileRead("/admin.html");
}

bool BrowserServerClass::saveHTTPAuth() {
	
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

/*
void BrowserServerClass::send_wwwauth_configuration_values_html() {
	String values = "";

	values += "wwwauth|" + (String)(_httpAuth.auth ? "checked" : "") + "|chk\n";
	values += "wwwuser|" + (String)_httpAuth.wwwUsername + "|input\n";
	values += "wwwpass|" + (String)_httpAuth.wwwPassword + "|input\n";

	send(200, "text/plain", values);
}*/

bool BrowserServerClass::checkAuth() {
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

bool BrowserServerClass::loadHTTPAuth() {
    File configFile = SPIFFS.open(SECRET_FILE, "r");
    if (!configFile) {
        _httpAuth.auth = false;
        _httpAuth.wwwUsername = "";
        _httpAuth.wwwPassword = "";
        configFile.close();
        return false;
    }

    size_t size = configFile.size();    

    // Allocate a buffer to store contents of the file.
    std::unique_ptr<char[]> buf(new char[size]);

    // We don't use String here because ArduinoJson library requires the input
    // buffer to be mutable. If you don't use ArduinoJson, you may as well
    // use configFile.readString instead.
    configFile.readBytes(buf.get(), size);
    configFile.close();
    DynamicJsonBuffer jsonBuffer(256);
    //StaticJsonBuffer<256> jsonBuffer;
    JsonObject& json = jsonBuffer.parseObject(buf.get());

    if (!json.success()) {
        _httpAuth.auth = false;
        return false;
    }
    _httpAuth.auth = json["auth"];
    _httpAuth.wwwUsername = json["user"].as<char*>();
    _httpAuth.wwwPassword = json["pass"].as<char*>();    
    return true;
}

void BrowserServerClass::setUpdateMD5() {
	_browserMD5 = "";
	if (args() > 0){  // Read hash
		for (uint8_t i = 0; i < args(); i++) {			
			if (argName(i) == "md5") {
				_browserMD5 = urldecode(arg(i));
				Update.setMD5(_browserMD5.c_str());
				continue;
			}if (argName(i) == "size") {
				_updateSize = arg(i).toInt();				
				continue;
			}
		}
		send(200, "text/html", "OK --> MD5: " + _browserMD5);
	}

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
	if(!browserServer.hasArg("dir")) {browserServer.send(500, "text/plain", "BAD ARGS"); return;}
	
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

String BrowserServerClass::getContentType(String filename){
	if(hasArg("download")) return "application/octet-stream";
	else if(filename.endsWith(".htm")) return "text/html";
	else if(filename.endsWith(".html")) return "text/html";
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
	return false;	
}








