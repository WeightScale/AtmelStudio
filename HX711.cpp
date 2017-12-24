#include <Arduino.h>
//#include <FIR.h>
#include "HX711.h"
#include "Filter.h"

ExponentialFilter<long> ADCFilter;
//FIR<d_type, 8> fir;
//d_type coef[8] = { 1., 1., 1., 1., 1., 1., 1., 1.};
//d_type coef[32] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
//d_type coef[5] = { 0.07779184963279696,	0.15558369926559376,0.19447953885131547,0.15558369926559376,0.07779184963279696/*,  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1*/};
		

HX711::HX711(byte dout, byte pd_sck, byte gain) {
	PD_SCK 	= pd_sck;
	DOUT 	= dout;

	GAIN = 1;
	pinsConfigured = false;
	ADCFilter.SetWeight(20);
	ADCFilter.SetCurrent(0);
	//fir.setFilterCoeffs(coef);
}

HX711::~HX711(){

}

bool HX711::is_ready() {
	if (!pinsConfigured) {
		// We need to set the pin mode once, but not in the constructor
		pinMode(PD_SCK, OUTPUT);
		pinMode(DOUT, INPUT);
		pinsConfigured = true;
		tare();
	}
	return digitalRead(DOUT) == LOW;
}

void HX711::set_gain(byte gain) {
	switch (gain) {
		case 128:		// channel A, gain factor 128
			GAIN = 1;
			break;
		case 64:		// channel A, gain factor 64
			GAIN = 3;
			break;
		case 32:		// channel B, gain factor 32
			GAIN = 2;
			break;
	}

	digitalWrite(PD_SCK, LOW);
	read();
}

long HX711::read() {
	// wait for the chip to become ready
	while (!is_ready());

    unsigned long value = 0;
    byte data[3] = { 0 };
    byte filler = 0x00;

	// pulse the clock pin 24 times to read the data
    data[2] = shiftIn(DOUT, PD_SCK, MSBFIRST);
    data[1] = shiftIn(DOUT, PD_SCK, MSBFIRST);
    data[0] = shiftIn(DOUT, PD_SCK, MSBFIRST);

	// set the channel and the gain factor for the next reading using the clock pin
	for (unsigned int i = 0; i < GAIN; i++) {
		digitalWrite(PD_SCK, HIGH);
		digitalWrite(PD_SCK, LOW);
	}

    // Datasheet indicates the value is returned as a two's complement value
    // Flip all the bits
    data[2] = ~data[2];
    data[1] = ~data[1];
    data[0] = ~data[0];

    // Replicate the most significant bit to pad out a 32-bit signed integer
    if ( data[2] & 0x80 ) {
        filler = 0xFF;
    } else if ((0x7F == data[2]) && (0xFF == data[1]) && (0xFF == data[0])) {
        filler = 0xFF;
    } else {
        filler = 0x00;
    }

    // Construct a 32-bit signed integer
    value = ( static_cast<unsigned long>(filler) << 24
            | static_cast<unsigned long>(data[2]) << 16
            | static_cast<unsigned long>(data[1]) << 8
            | static_cast<unsigned long>(data[0]) );

    // ... and add 1
    return static_cast<long>(++value);
}

long HX711::read_average(byte times) {
	long long sum = 0;
	for (byte i = 0; i < times; i++) {
		ADCFilter.Filter(read());
		sum += ADCFilter.Current();
	}	
	return times == 0?sum / 1:sum / times;
	//return fir.processReading(times == 0?sum / 1:sum / times);
	//return times == 0?sum / 1:sum / times;
}

long HX711::get_value() {
	//return fir.processReading(read_average(FILTER) - OFFSET);
	return read_average(FILTER) - OFFSET;
}

d_type HX711::get_units() {
	d_type v = get_value();
	//return fir.processReading(v / SCALE);
	return (v / SCALE);
}

void HX711::tare() {
	long sum = read_average(FILTER);
	set_offset(sum);
}

void HX711::power_down() {
	digitalWrite(PD_SCK, HIGH);
}

void HX711::power_up() {
	digitalWrite(PD_SCK, LOW);
}

void HX711::set_filter(unsigned char f){
	FILTER = constrain(f, 1, 10);
	/*FILTER = f;
	if (f < 1)
		FILTER = 1;
	else if(f > 10)
		FILTER = 10;*/
}

void HX711::reset(){
	power_down();
	delayMicroseconds(100);
	power_up();
}

/*
void HX711::stable(){
	int V[4]={0};
	unsigned char i;
	int Vmax, Vmin;
	
	//Loop until the ADC value is stable. (Vmax <= (Vmin+1))
	for (Vmax=10,Vmin= 0;Vmax > (Vmin+6);){
		V[3] = V[2];
		V[2] = V[1];
		V[1] = V[0];
		while (!is_ready());
		V[0] = read();
		Vmin = V[0];                          // Vmin is the lower VOLTAGE
		Vmax = V[0];                          // Vmax is the higher VOLTAGE
		/ *Save the max and min voltage* /
		for (i=0;i<=3;i++){
			if (V[i] > Vmax)
				Vmax=V[i];
			if (V[i] < Vmin)
				Vmin=V[i];
		}
	}
}*/






