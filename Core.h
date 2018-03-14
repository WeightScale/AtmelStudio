// scales.h

#ifndef _CORE_h
#define _CORE_h

#include "TaskController.h"
#include "Task.h"

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif
#include <ArduinoJson.h>
#include "Scale.h"
//using namespace ArduinoJson;

#define SETTINGS_FILE "/settings.json"
#define HOST_URL "sdb.net.ua"
#define TIMEOUT_HTTP 3000
#define STABLE_NUM_MAX 10
#define MAX_EVENTS 100
//#define MAX_CHG 1013//980	//делитель U2=U*(R2/(R1+R2)) 0.25
#define MIN_CHG 768			//ADC = (Vin * 1024)/Vref  Vref = 1V

#define EN_NCP  12							/* сигнал включения питания  */
#define PWR_SW  13							/* сигнал от кнопки питания */
#define LED  2								/* индикатор работы */

#define SCALE_JSON		"scale"
#define SERVER_JSON		"server"
//#define DATE_JSON		"date"
#define EVENTS_JSON		"events"

extern TaskController taskController;		/*  */
extern Task taskBlink;								/*  */
extern Task taskBattery;							/*  */
extern Task taskPower;
extern void connectWifi();

typedef struct {	
	bool autoIp;
	String scaleName;
	String scalePass;
	String scaleLanIp;
	String scaleGateway;
	String scaleSubnet;
	String scaleWlanSSID;
	String scaleWlanKey;
	String hostUrl;
	String hostPin;
	int timeout;
	int bat_max;	
} settings_t;

class CoreClass /*: public HX711, public ScaleMemClass*/{
	protected:
	settings_t _settings;
	unsigned int _charge;
	
	bool saveAuth();
	bool loadAuth();		
	bool _downloadSettings();
	void _callibratedBaterry();		

	public:			
		CoreClass();
		~CoreClass();
		void begin();
		bool saveSettings();
		String& getNameAdmin(){return _settings.scaleName;};
		String& getPassAdmin(){return _settings.scalePass;};
		String& getSSID(){return _settings.scaleWlanSSID;};
		String& getLanIp(){return _settings.scaleLanIp;};
		String& getGateway(){return _settings.scaleGateway;};
		void setSSID(const String& ssid){_settings.scaleWlanSSID = ssid;};
		void setPASS(const String& pass){_settings.scaleWlanKey = pass;};	
		String& getPASS(){return _settings.scaleWlanKey;};
		bool saveEvent(const String&, const String&);
		String getIp();
		bool eventToServer(const String&, const String&, const String&);
		void saveValueSettingsHttp(AsyncWebServerRequest*);	
		void handleSetAccessPoint(AsyncWebServerRequest*);	
		String getHash(const String&, const String&, const String&, const String&);
		int getBattery(int);		
		void setCharge(unsigned int ch){_charge = ch;};
		unsigned int getCharge(){return _charge;};
		bool isAuto(){return _settings.autoIp;};
		int getADC(byte times = 1);
		
		
};

void powerOff();
void reconnectWifi(AsyncWebServerRequest*);
extern CoreClass CORE;

#endif //_CORE_h







