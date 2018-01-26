
#ifndef _SCALE_h
#define _SCALE_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "Arduino.h"
#else
	#include "WProgram.h"
#endif
#include "HX711.h"
#include "Filter.h"
#include "BrowserServer.h"

#define PAGE_FILE	"/calibr.html"
#define CDATE_FILE	"/cdate.json"

typedef struct {
	long offset;		/*  */
	unsigned char average;				/*  */
	unsigned char step;					/*  */
	int accuracy;					/*  */
	//unsigned char w_filter; /*! Значение для фильтра от 1-100 % */
	unsigned int max;					/*  */
	d_type scale;
}t_scales_value;

class BrowserServerClass;

class ScaleClass : public HX711 , public ExponentialFilter<long>{
	protected:
		BrowserServerClass *_server;
		char * _username;
		char * _password;
		bool _authenticated;
		t_scales_value _scales_value;
		d_type _round;						/* множитиль для округления */
		d_type _stable_step;				/* шаг для стабилизации */
		d_type _referenceWeight;			/*  */
		long _calibrateWeightValue;			/*  */		
			
		bool _downloadValue();

	public:
		ScaleClass(byte, byte);
		~ScaleClass();
		void setup(BrowserServerClass *server, const char * username, const char * password);	
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
		
		d_type getUnits();
		d_type getWeight();
		void tare();
		
		void formatValue(d_type value, char* string);
		d_type getStableStep(){return _stable_step;};
		
		d_type getRound(){return _round;};
		//void setMax(unsigned int m){_scales_value.max = m;};
		//void setStep(unsigned char s){_scales_value.step = s;};
		//void setAccuracy(int a){_scales_value.accuracy = a;};
		//void setCalibrateZeroValue(long z){_calibrateZeroValue = z;};
		//void setCalibrateWeightValue(long w){_calibrateWeightValue = w;};	
		//void setReferenceWeight(d_type r){_referenceWeight = r;};			
};

extern ScaleClass Scale;

#endif

