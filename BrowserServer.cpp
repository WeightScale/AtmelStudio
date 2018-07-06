#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFSEditor.h>
#include <ArduinoJson.h>
#include <Hash.h>
#include <AsyncJson.h>
#include <functional>
#include "BrowserServer.h"
#include "tools.h"
#include "Core.h"
#include "Version.h"
#include "DateTime.h"
#include "HttpUpdater.h"
#include "web_server_config.h"

/* */
//ESP8266HTTPUpdateServer httpUpdater;
/* Soft AP network parameters */
IPAddress apIP(192,168,4,1);
IPAddress netMsk(255, 255, 255, 0);

IPAddress lanIp;			// Надо сделать настройки ip адреса
IPAddress gateway;

BrowserServerClass browserServer(80);
AsyncWebSocket ws("/ws");
DNSServer dnsServer;
//holds the current upload
//File fsUploadFile;

/* hostname for mDNS. Should work at least on windows. Try http://esp8266.local */


BrowserServerClass::BrowserServerClass(uint16_t port) : AsyncWebServer(port) {}

BrowserServerClass::~BrowserServerClass(){}
	
void BrowserServerClass::begin() {
	/* Setup the DNS server redirecting all the domains to the apIP */
	dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
	dnsServer.start(DNS_PORT, "*", apIP);	
	_downloadHTTPAuth();
	ws.onEvent(onWsEvent);
	addHandler(&ws);
	CORE = new CoreClass(_httpAuth.wwwUsername.c_str(), _httpAuth.wwwPassword.c_str());
	addHandler(CORE);
	addHandler(new CaptiveRequestHandler()).setFilter(ON_AP_FILTER);
	addHandler(new SPIFFSEditor(_httpAuth.wwwUsername.c_str(), _httpAuth.wwwPassword.c_str()));	
	addHandler(new HttpUpdaterClass("sa", "654321"));
	init();
	AsyncWebServer::begin(); // Web server start
}

void BrowserServerClass::init(){
	on("/settings.json",HTTP_ANY, handleFileReadAuth);
	on("/rc", reconnectWifi);									/* Пересоединиться по WiFi. */
	on("/sv", handleScaleProp);									/* Получить значения. */	
	/*on("/heap", HTTP_GET, [](AsyncWebServerRequest *request){
		request->send(200, "text/plain", String(ESP.getFreeHeap()));
	});*/
	on("/secret.json",[](AsyncWebServerRequest * reguest){
		if (!browserServer.isAuthentified(reguest)){
			return reguest->requestAuthentication();
		}
		reguest->send(SPIFFS, reguest->url());	
	});
#ifdef HTML_PROGMEM
	on("/",[](AsyncWebServerRequest * reguest){	reguest->send_P(200,F("text/html"),index_html);});								/* Главная страница. */
	rewrite("/sn", "/settings.html");
	serveStatic("/", SPIFFS, "/");
#else
	serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");
#endif	
		
	onNotFound([](AsyncWebServerRequest *request){
		request->send(404);
	});
}

bool BrowserServerClass::_downloadHTTPAuth() {
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
	_httpAuth.wwwUsername = json["user"].as<String>();
	_httpAuth.wwwPassword = json["pass"].as<String>();
	return true;
}

bool BrowserServerClass::checkAdminAuth(AsyncWebServerRequest * r) {	
	return r->authenticate(_httpAuth.wwwUsername.c_str(), _httpAuth.wwwPassword.c_str());
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

bool BrowserServerClass::isAuthentified(AsyncWebServerRequest * request){
	if (!request->authenticate(CORE->getNameAdmin().c_str(), CORE->getPassAdmin().c_str())){
		if (!checkAdminAuth(request)){
			return false;
		}
	}
	return true;
}

void handleFileReadAuth(AsyncWebServerRequest * request){
	if (!browserServer.isAuthentified(request)){
		return request->requestAuthentication();
	}
	request->send(SPIFFS, request->url());
}

void handleScaleProp(AsyncWebServerRequest * request){
	if (!browserServer.isAuthentified(request))
	return request->requestAuthentication();
	AsyncJsonResponse * response = new AsyncJsonResponse();
	JsonObject& root = response->getRoot();
	root["id_date"] = getDateTime();
	root["id_local_host"] = String(MY_HOST_NAME);
	root["id_ap_ssid"] = String(SOFT_AP_SSID);
	root["id_ap_ip"] = toStringIp(WiFi.softAPIP());
	root["id_ip"] = toStringIp(WiFi.localIP());
	root["sl_id"] = String(Scale.getSeal());					
	root["chip_v"] = String(ESP.getCpuFreqMHz());
	root["id_mac"] = WiFi.macAddress();
	response->setLength();
	request->send(response);
}

void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len){
	if(type == WS_EVT_CONNECT){
		client->ping();
		}else if(type == WS_EVT_DATA){
		String msg = "";
		for(size_t i=0; i < len; i++) {
			msg += (char) data[i];
		}
		if (msg.equals("/wt")){
			DynamicJsonBuffer jsonBuffer;
			JsonObject& json = jsonBuffer.createObject();
			size_t len = Scale.doData(json);							
			AsyncWebSocketMessageBuffer * buffer = ws.makeBuffer(len);
			if (buffer) {
				json.printTo((char *)buffer->get(), len + 1);
				if (client) {
					client->text(buffer);
				}
			}
			POWER.updateCache();
		}
	}
}