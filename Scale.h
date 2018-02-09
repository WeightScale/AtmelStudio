
#ifndef _SCALE_h
#define _SCALE_h

/*
#if defined(ARDUINO) && ARDUINO >= 100
	#include "Arduino.h"
#else
	#include "WProgram.h"
#endif*/
#include "HX711.h"
#include "Filter.h"
#include "BrowserServer.h"

#define PAGE_FILE			"/calibr.html"
#define CDATE_FILE			"/cdate.json"
#define WEIGHT_MAX_JSON		"mw_id"
#define OFFSET_JSON			"ofs"
#define AVERAGE_JSON		"av_id"
#define STEP_JSON			"st_id"
#define ACCURACY_JSON		"ac_id"
#define SCALE_JSON			"scale"
#define FILTER_JSON			"fl_id"
#define SEAL_JSON			"sl_id"
#define USER_JSON			"us_id"
#define PASS_JSON			"ps_id"

typedef struct {
	long offset;		/*  */
	unsigned char average;				/*  */
	unsigned char step;					/*  */
	int accuracy;					/*  */
	//unsigned char w_filter; /*! �������� ��� ������� �� 1-100 % */
	unsigned int max;					/*  */
	float scale;
	int seal;
	String user;
	String password;
}t_scales_value;

class BrowserServerClass;

class ScaleClass : public HX711 , public ExponentialFilter<long>{
	protected:
		BrowserServerClass *_server;
		//char * _username;
		//char * _password;
		bool _authenticated;
		t_scales_value _scales_value;
		float _round;						/* ��������� ��� ���������� */
		float _stable_step;					/* ��� ��� ������������ */
		float _referenceWeight;				/*  */
		long _calibrateWeightValue;			/*  */		
			
		bool _downloadValue();

	public:
		ScaleClass(byte, byte);
		~ScaleClass();
		void setup(BrowserServerClass *server/*, const char * username, const char * password*/);	
		bool saveDate();
		void saveValueCalibratedHttp();
		void mathScale();
		void mathRound();
		//void setScale(d_type scale = 1.f){_scales_value.scale = scale;};
		//d_type getScale(){return _scales_value.scale;};
			
		void setOffset(long offset = 0){_scales_value.offset = offset;};
		long getOffset(){return _scales_value.offset;};
		void init();	
		long readAverage();
		long getValue();
		void setAverage(unsigned char);
		unsigned char getAverage(){return _scales_value.average;};	
		void setSeal(int s){_scales_value.seal = s; };
		int getSeal(){ return _scales_value.seal;};	
		BrowserServerClass *getServer(){ return _server;};
		
		float getUnits();
		float getWeight();
		void tare();
		
		void formatValue(float value, char* string);
		float getStableStep(){return _stable_step;};
		
		float getRound(){return _round;};
		
		//void setMax(unsigned int m){_scales_value.max = m;};
		//void setStep(unsigned char s){_scales_value.step = s;};
		//void setAccuracy(int a){_scales_value.accuracy = a;};
		//void setCalibrateZeroValue(long z){_calibrateZeroValue = z;};
		//void setCalibrateWeightValue(long w){_calibrateWeightValue = w;};	
		//void setReferenceWeight(d_type r){_referenceWeight = r;};			
};

extern ScaleClass Scale;
void handleSeal();

#endif
