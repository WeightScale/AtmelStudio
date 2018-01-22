//#include <EEPROM.h>
#include "handleHttp.h"
#include "tools.h"
#include "Scales.h"
#include "Page.h"
#include "DateTime.h"
#include "BrowserServer.h"

void handleScaleProp(){	
	if (!isAuthentified())
		return browserServer.requestAuthentication();
	String values = "";
	values += "id_date|" + getDateTime() + "|div\n";	
	values += "id_local_host|http://"+String(MY_HOST_NAME)+".local|div\n";
	values += "id_ap_ssid|" + String(SOFT_AP_SSID) + "|div\n";
	values += "id_ap_ip|" + toStringIp(WiFi.softAPIP()) + "|div\n";
	values += "id_ip|" + toStringIp(WiFi.localIP()) + "|div\n";
	browserServer.send(200, "text/plain", values);
}

/** Handle the WLAN save form and redirect to WLAN config page again */
void handlePropSave() {
	bool rec = false;
	
		
	if (browserServer.args() > 0){ // Save Settings			
		SCALES.sendScaleSettingsSaveValue();		
	}else{
		handleFileRead(browserServer.uri());
	}	
}

void handleCalibrSave(){
	if (browserServer.args() > 0){ // Save Settings			
		SCALES.scaleCalibrateSaveValue();		
	}else{
		handleFileRead(browserServer.uri());
	}			
}

//Check if header is present and correct
bool isAuthentified(){
	if (!browserServer.authenticate(SCALES.getNameAdmin().c_str(), SCALES.getPassAdmin().c_str())){
		if (!browserServer.checkAuth()){
			return false;	
		}
	}	
	return true;
}









