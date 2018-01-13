// BrowserServer.h

#ifndef _BROWSERSERVER_h
#define _BROWSERSERVER_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif
#include <ESP8266WiFi.h>
#include <IPAddress.h>
#include <WiFiClient.h>
//#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <ESP8266mDNS.h>
#include <FS.h>
//#include <ArduinoOTA.h>
#include <ArduinoJson.h>
#include "Scales.h"

#define countof(a) (sizeof(a) / sizeof(a[0]))
#define SECRET_FILE "/secret.json"

// DNS server
const byte DNS_PORT = 53;

//typedef std::function<void(double)> HandlerFunction_t;
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
		bool saveHTTPAuth();
		//void send_wwwauth_configuration_values_html();		
		bool loadHTTPAuth();
		static String urldecode(String input); // (based on https://code.google.com/p/avr-netino/)
		static unsigned char h2int(char c);

	public:
		BrowserServerClass(uint16_t port);
		~BrowserServerClass();
		void begin();
		void init();
		void send_update_firmware_values_html();
		void send_wwwauth_configuration_html();
		void restart_esp();		
		String getContentType(String filename);	
		bool isValidType(String filename);	
		void setUpdateMD5();
		bool checkAuth();
		
		friend ScalesClass;
};

/** Should I connect to WLAN asap? */
extern boolean connect;

//extern const char* super_user_login;
//extern const char* super_user_password;

/* hostname for mDNS. Should work at least on windows. Try http://esp8266.local */
extern const char *myHostname;
/* Set these to your desired softAP credentials. They are not configurable at runtime */
extern const char *softAP_ssid;
extern const char *softAP_password;

//extern ESP8266HTTPUpdateServer httpUpdater;
extern DNSServer dnsServer;
extern IPAddress apIP;
extern IPAddress netMsk;
extern BrowserServerClass browserServer;

bool handleFileRead(String path);
void handleFileCreate();
void handleFileDelete();
void handleFileUpload();
void handleFileList();

#endif







