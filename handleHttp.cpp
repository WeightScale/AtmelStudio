#include "handleHttp.h"
#include "tools.h"
#include "Core.h"
#include "DateTime.h"
#include "BrowserServer.h"

void handleScaleProp(){	
	if (!browserServer.isAuthentified())
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
void handleSettingsHtml() {	
	if (!browserServer.isAuthentified())
		return browserServer.requestAuthentication();
	if (browserServer.args() > 0) // Save Settings
		return CORE.saveValueSettingsHttp();	
	handleFileRead(browserServer.uri());
}

/*
void handleCalibratedHtml(){
	if (browserServer.args() > 0) // Save Settings			
		return CORE.saveValueCalibratedHttp();		
	handleFileRead(browserServer.uri());
}*/

//Check if header is present and correct
/*
bool isAuthentified(){
	if (!browserServer.authenticate(CORE.getNameAdmin().c_str(), CORE.getPassAdmin().c_str())){
		if (!browserServer.checkAdminAuth()){
			return false;	
		}
	}	
	return true;
}*/









