#include "CoreMemory.h"

void CoreMemoryClass::init(){
	String u = "admin";
	String p = "1234";	
	//eeprom.scales_value.password = "1234";
	begin(sizeof(MyEEPROMStruct));
	if (percentUsed() >= 0) {
		get(0, eeprom);
	}else{
		u.toCharArray(eeprom.scales_value.user, u.length()+1);	
		p.toCharArray(eeprom.scales_value.password, p.length()+1);
		u.toCharArray(eeprom.settings.scaleName, u.length()+1);
		p.toCharArray(eeprom.settings.scalePass, p.length()+1);
	}
}

bool CoreMemoryClass::save(){
	put(0, eeprom);
	return commit();
}


CoreMemoryClass CoreMemory;

