// scales.h

#ifndef _SCALES_h
#define _SCALES_h

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
#define STABLE_NUM_MAX 20
#define MAX_EVENTS 100
#define MAX_CHG 1013//980	//делитель U2=U*(R2/(R1+R2)) 0.25
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
} settings_t;

class CoreClass /*: public HX711, public ScaleMemClass*/{
	protected:
	settings_t _settings;
	unsigned int charge;
	
	bool saveAuth();
	bool loadAuth();
	bool _saveSettings();	
	bool _downloadSettings();		

	public:			
		CoreClass();
		~CoreClass();
		void begin();
		String& getNameAdmin(){return _settings.scaleName;};
		String& getPassAdmin(){return _settings.scalePass;};
		String& getSSID(){return _settings.scaleWlanSSID;};
		String& getLanIp(){return _settings.scaleLanIp;};
		String& getGateway(){return _settings.scaleGateway;};
		void setSSID(const String&);
		String& getPASS(){return _settings.scaleWlanKey;};
		void setPASS(const String&);
		bool saveEvent(const String&, const String&);
		String getIp();
		bool eventToServer(const String&, const String&, const String&);
		void saveValueSettingsHttp(const char * text);		
		String getHash(const String&, const String&, const String&, const String&);
		int getBattery(int);
		void detectStable(d_type);
		void setCharge(unsigned int ch){charge = ch;};
		unsigned int getCharge(){return charge;};
		bool isAuto(){return _settings.autoIp;};
		
};

void powerOff();
void reconnectWifi();
extern CoreClass CORE;

#endif







