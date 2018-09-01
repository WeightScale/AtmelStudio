
#include "SaveWeight.h"
#include "Core.h"


void SaveWeightClass::handleRequest(AsyncWebServerRequest *request) {
	File readFile = SPIFFS.open("/events.json", "r");
	if (!readFile) {
		return;
	}
	
	size_t size = readFile.size();
	std::unique_ptr<char[]> buf(new char[size]);
	readFile.readBytes(buf.get(), size);
	readFile.close();
	
	DynamicJsonBuffer jsonBuffer(JSON_ARRAY_SIZE(110));
	//DynamicJsonBuffer jsonBuffer;
	JsonObject& json = jsonBuffer.parseObject(buf.get());

	if (!json.containsKey(EVENTS_JSON)) {
		json["cur_num"] = 0;
		json["max_events"] = MAX_EVENTS;
		JsonArray& events = json.createNestedArray(EVENTS_JSON);
		for(int i = 0; i < MAX_EVENTS; i++){
			JsonObject& ev = jsonBuffer.createObject();
			ev["d"] = "";
			ev["v"] = "";
			ev["s"] = false;
			events.add(ev);
		}
	}
	
	long n = json["cur_num"];
	
	json[EVENTS_JSON][n]["d"] = date;
	json[EVENTS_JSON][n]["v"] = value;
	json[EVENTS_JSON][n]["s"] = flag;
	
	if ( ++n == MAX_EVENTS){
		n = 0;
	}
	json["max_events"] = MAX_EVENTS;
	json["cur_num"] = n;
	//TODO add AP data to html
	File saveFile = SPIFFS.open("/events.json", "w");
	if (!saveFile) {
		return;
	}

	json.printTo(saveFile);
	saveFile.close();
}

bool SaveWeightClass::canHandle(AsyncWebServerRequest *request){
	if (isSave){
		date = getDateTime();
		flag = WiFi.status() == WL_CONNECTED?CORE->eventToServer(date, String("weight"), String(value)):false;
		isSave = false;
		return true;
	}
	return false;
}

SaveWeightClass * SaveWeight;

