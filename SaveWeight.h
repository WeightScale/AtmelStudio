// SaveWeight.h

#ifndef _SAVEWEIGHT_h
#define _SAVEWEIGHT_h
#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

class SaveWeightClass : public AsyncWebHandler {
 protected:
	bool isSave;
	float value;
	long int time;
	String date;
	bool flag;
 public:
	SaveWeightClass() {}
	virtual ~SaveWeightClass() {}

	virtual bool canHandle(AsyncWebServerRequest *request) override final;
	virtual void handleRequest(AsyncWebServerRequest *request) override final;
	virtual bool isRequestHandlerTrivial() override final {return false;}
	 
};

extern SaveWeightClass * SaveWeight;

#endif

