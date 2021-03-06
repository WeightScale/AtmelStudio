#ifndef HX711_h
#define HX711_h

#include "Filter.h"

#if ARDUINO >= 100
	#include "Arduino.h"
#else
	#include "WProgram.h"
#endif

typedef float d_type;

class HX711 {
	private:
		byte PD_SCK;	// Power Down and Serial Clock Input Pin
		byte DOUT;		// Serial Data Output Pin
		byte GAIN;		// amplification factor
		long OFFSET;	// used for tare weight
		d_type SCALE;	// used to return weight in grams, kg, ounces, whatever
		unsigned char FILTER;
		//unsigned char STEP;		
		bool pinsConfigured;

	public:
		// define clock and data pin, channel, and gain factor
		// channel selection is made by passing the appropriate gain: 128 or 64 for channel A, 32 for channel B
		// gain: 128 or 64 for channel A; channel B works with 32 gain factor only
		HX711(byte dout, byte pd_sck, byte gain = 128);
		virtual ~HX711();
		// check if HX711 is ready
		// from the datasheet: When output data is not ready for retrieval, digital output pin DOUT is high. Serial clock
		// input PD_SCK should be low. When DOUT goes to low, it indicates data is ready for retrieval.
		bool is_ready();
		// set the gain factor; takes effect only after a call to read()
		// channel A can be set for a 128 or 64 gain; channel B has a fixed 32 gain
		// depending on the parameter, the channel is also set to either A or B
		void set_gain(byte gain = 128);
		// waits for the chip to be ready and returns a reading
		long read();
		// returns an average reading; times = how many times to read
		long read_average(byte times = 3);
		// returns (read_average() - OFFSET), that is the current value without the tare weight; times = how many readings to do
		long get_value();
		// returns get_value() divided by SCALE, that is the raw value divided by a value obtained via calibration
		// times = how many readings to do
		d_type get_units();
		// set the OFFSET value for tare weight; times = how many times to read the tare value
		void tare();
		// set the SCALE value; this value is used to convert the raw data to "human readable" data (measure units)
		void set_scale(d_type scale = 1.f){SCALE = scale;};
		// get the current SCALE
		d_type get_scale(){return SCALE;};
		// set OFFSET, the value that's subtracted from the actual reading (tare weight)
		void set_offset(long offset = 0){OFFSET = offset;};
		// get the current OFFSET
		long get_offset(){return OFFSET;};
		// puts the chip into power down mode
		void power_down();
		// wakes up the chip after power down mode
		void power_up();		
		void set_filter(unsigned char);		
		//unsigned char get_filter(){return FILTER;};		
		//void set_step(unsigned char step = 1){STEP = step;};		
		//unsigned char get_step(){return STEP;};	
		//void setFWEIGHT(unsigned char f){fWEIGHT = constrain(f, 1, 100);};		
		void reset();
		//void stable();
};

extern ExponentialFilter<long> ADCFilter;

#endif /* HX711_h */






