#ifndef _BROWSERSERVER_h
#define _BROWSERSERVER_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif
#include <IPAddress.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <FS.h>
#include <ArduinoJson.h>
#include "Core.h"

#define SECRET_FILE "/secret.json"

#define MY_HOST_NAME "scales"
#define SOFT_AP_SSID "SCALES"
#define SOFT_AP_PASSWORD "12345678"

// DNS server
#define DNS_PORT 53

typedef struct {
	bool auth;
	String wwwUsername;
	String wwwPassword;
} strHTTPAuth;

class ESP8266WebServer;

class BrowserServerClass : public ESP8266WebServer{
	protected:
		strHTTPAuth _httpAuth;
		String _browserMD5 = "";
		uint32_t _updateSize = 0;
		bool _saveHTTPAuth();		
		bool _downloadHTTPAuth();
		static String urldecode(String input); // (based on https://code.google.com/p/avr-netino/)
		static unsigned char h2int(char c);

	public:
	
		BrowserServerClass(uint16_t port);
		~BrowserServerClass();
		void begin();
		void init();
		void send_update_firmware_values_html();
		void send_wwwauth_configuration_html();
		//void restart_esp();		
		String getContentType(String filename);	
		bool isValidType(String filename);	
		void setUpdateMD5();
		bool checkAuth();
		
		friend CoreClass;
};

//extern ESP8266HTTPUpdateServer httpUpdater;
extern DNSServer dnsServer;
extern IPAddress apIP;
extern IPAddress netMsk;
extern IPAddress lanIp;			// Надо сделать настройки ip адреса
extern IPAddress gateway;
extern BrowserServerClass browserServer;

bool handleFileRead(String path);
void handleFileCreate();
void handleFileDelete();
void handleFileUpload();
void handleFileList();

#endif







