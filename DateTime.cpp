// 
// 
// 
#include "DateTime.h"
//#include "RtcDateTime.h"
#include "BrowserServer.h"

//uRTCLib rtc;
//RtcDS3231<TwoWire> Rtc(Wire);
RtcDS1307<TwoWire> Rtc(Wire);

DateTimeClass::DateTimeClass(const String& date){
	
	_dayOfMonth = date.substring(0, 2).toInt();
	_month = date.substring(3, 5).toInt();
	_year = date.substring(6, date.indexOf(',')).toInt();
	_hour = date.substring(12, 14).toInt();
	_minute = date.substring(15, 17).toInt();
	_second = date.substring(18).toInt();	
	
}

DateTimeClass::~DateTimeClass(){
	//Serial.println("destruct");
}


RtcDateTime DateTimeClass::toRtcDateTime(){
	return RtcDateTime(_year, _month, _dayOfMonth, _hour, _minute, _second);
	//return NULL;
}

String getDateTime(){
	char datestring[20];		
	RtcDateTime now = Rtc.GetDateTime();
	snprintf_P(datestring,countof(datestring),PSTR("%04u.%02u.%02u-%02u:%02u:%02u"),
	now.Year(),
	now.Month(),
	now.Day(),	
	now.Hour(),
	now.Minute(),
	now.Second() );
	return String(datestring);
}


//DateTimeClass DateTime;







