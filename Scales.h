// scales.h

#ifndef _SCALES_h
#define _SCALES_h

//#include <ThreadController.h>
//#include <Thread.h>
//#include <StaticThreadController.h>
#include "TaskController.h"
#include "Task.h"

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif
#include <ArduinoJson.h>
//#include "ScaleMem.h"
#include "HX711.h"
using namespace ArduinoJson;

#define SETTINGS_FILE "/settings.json"
#define STABLE_NUM_MAX 10
#define STABLE_DELTA_STEP 10
#define MAX_EVENTS 100
#define MAX_CHG 1013//980 
#define MIN_CHG 720
//#define DIAPAZONE (MAX_CHG - MIN_CHG)

#define EN_NCP  12							/* сигнал включения питания  */
#define PWR_SW  13							/* сигнал от кнопки питания */
#define LED  2								/* индикатор работы */

extern TaskController taskController;		/*  */
extern Task taskBlink;								/*  */
extern Task taskBattery;							/*  */
extern Task taskPower;
extern void connectWifi();

typedef struct {
	//unsigned int pwr_time;			/*  */
	long adc_offset;		/*  */
	unsigned char filter;				/*  */
	unsigned char step;					/*  */
	int accuracy;					/*  */
	//unsigned char w_filter; /*! Значение для фильтра от 1-100 % */
	unsigned int max;					/*  */
	d_type scale;
	String scaleName;
	String scalePass;
	String scaleWlanSSID;
	String scaleWlanKey;
	String hostUrl;
	String hostPin;	
} settings_t;

class ScalesClass : public HX711/*, public ScaleMemClass*/{
	protected:	
	d_type _weight=-1, _weight_temp;
	d_type _round; /* множитиль для округления */
	d_type _stable_step; /* шаг для стабилизации */
	unsigned char _stable_num = 0;
	bool isStable = false;
	settings_t _settings;
	long calibrateZero;				/*  */
	long calibrateWeight;			/*  */
	d_type accurateWeight;	/*  */
	//double tempScale;				/*  */
	unsigned int charge;
	//unsigned char percent;
	void mathScale();
	bool saveAuth();
	bool loadAuth();
	bool saveSettings();	
	bool loadSettings();		

	public:			
		ScalesClass(byte, byte/*, uint32_t*/);
		~ScalesClass();
		void begin();
		String& getNameAdmin(){return _settings.scaleName;};
		String& getPassAdmin(){return _settings.scalePass;};
		String& getSSID(){return _settings.scaleWlanSSID;};
		void setSSID(const String&);
		String& getPASS(){return _settings.scaleWlanKey;};
		void setPASS(const String&);
		bool saveEvent(const String&, const String&);
		//bool pingServer();
		String getIp();
		bool eventToServer(const String&, const String&, const String&);
		void sendScaleSettingsSaveValue();
		void scaleCalibrateSaveValue();
		String getHash(const String&, const String&, const String&, const String&);
		int getBattery(int);
		bool saveDate();
		void detectStable();
		void setWeight(d_type weight){_weight = round(weight * _round) / _round; };
		d_type getWeight(){return _weight;};
		int getAccuracy(){return _settings.accuracy;}
		unsigned int getMax(){return _settings.max;}
		void updateSettings();
		void setCharge(unsigned int ch){charge = ch;}
		unsigned int getCharge(){return charge;}
		void formatValue(d_type value, /*int digits, int accuracy,*/ char* string);
		unsigned char getFilter(){return _settings.filter;}	
		void mathRound(){
			_round = pow(10.0, _settings.accuracy) / _settings.step; // множитель для округления}
			_stable_step = 1/_round;} 
		d_type getRound(){return _round;};
		//d_type filter(d_type data, /*d_type prev_data,*/ d_type delta_data, d_type filter_step, d_type filter_cof);
			
		
		/*friend ScaleMemClass;*/
		friend HX711;	
};

void powerOff();
extern ScalesClass SCALES;

#endif







