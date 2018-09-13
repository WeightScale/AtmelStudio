// scales.h

#ifndef _CORE_h
#define _CORE_h
#include "web_server_config.h"
#include "TaskController.h"
#include "Task.h"

/*
#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif*/
#include <ArduinoJson.h>
#include "Scale.h"
#include "CoreMemory.h"
//using namespace ArduinoJson;

#define SETTINGS_FILE "/settings.json"




#define EN_NCP  12							/* сигнал включения питания  */
#define PWR_SW  13							/* сигнал от кнопки питания */
#define LED  2								/* индикатор работы */

#define SCALE_JSON		"scale"
#define SERVER_JSON		"server"
//#define DATE_JSON		"date"
#define EVENTS_JSON		"events"

extern TaskController taskController;		/*  */
extern Task taskBlink;								/*  */
extern Task taskBattery;							/*  */
extern Task taskConnectWiFi;
extern void connectWifi();

const char netIndex[] PROGMEM = R"(<html lang='en'><meta name='viewport' content='width=device-width, initial-scale=1, maximum-scale=1'/><body><form method='POST'><input name='ssid'><br/><input type='password' name='key'><br/><input type='submit' value='СОХРАНИТЬ'></form></body></html>)";

//settings.html
#if POWER_PLAN
	const char settings_html[] PROGMEM = R"(<!DOCTYPE html><html lang='en'><head> <meta charset='UTF-8'> <meta name='viewport' content='width=device-width, initial-scale=1, maximum-scale=1.0, user-scalable=no'/> <meta http-equiv="Cache-Control" content="no-cache, no-store, must-revalidate"/> <meta http-equiv="Pragma" content="no-cache"/> <title>Настройки</title> <link rel="stylesheet" type="text/css" href="global.css"> <style>input:focus{background: #FA6; outline: none;}table{width: 100%;}input, select{width: 100%; text-align: right; font-size: 18px;}input[type=submit]{width: auto;}input[type=checkbox]{width: auto;}</style> <script>window.onload=function (){load('global.css','css', function (){load('settings.js', 'js',function (){GetSettings();})});}; function load(e, t, n){let a; var d=document; if ('js'===t){a=d.createElement("script"); a.src=e; a.type='text/javascript'; a.async=!1; a.onload=function (){n()}; d.getElementsByTagName('head')[0].appendChild(a)}else if ('css'===t){a=d.createElement('link'); a.href=e; a.rel='stylesheet'; a.type='text/css'; a.async=!1; a.onload=function (){n()}; d.getElementsByTagName('head')[0].appendChild(a)}}</script></head><body style='visibility: hidden'><a href='/' class='btn btn--s btn--blue'>&lt;</a>&nbsp;&nbsp;<strong>Настройки</strong><hr><fieldset> <details> <summary>Конфигурация сети</summary> <br><h5 align='left'><b>Точка доступа WiFi роутера</b></h5> <form id='form_id' action='javascript:formNet("form_id")'> Получать IP: <input type='checkbox' id='id_auto' name='auto' onclick='enableAuthFields(this);'/> <div id='id_ip'></div><table id='id_table_net'> <tr> <td>IP:</td><td> <input id='id_lan_ip' type='text' name='lan_ip' onfocus='setOnBlur(this)'/> </td></tr><tr> <td>ШЛЮЗ:</td><td> <input id='id_gateway' type='text' name='gateway' onfocus='setOnBlur(this)'/> </td></tr><tr> <td>МАСКА:</td><td> <input id='id_subnet' type='text' name='subnet' onfocus='setOnBlur(this)'/> </td></tr></table> <table> <tr> <td>СЕТЬ:</td><td> <input id='id_ssid' name='ssid' placeholder='имя сети'> </td></tr><tr> <td>КЛЮЧ:</td><td> <input id='id_key' type='password' name='key' placeholder='пароль'> </td></tr><tr> <td/> <td> <input type='submit' value='СОХРАНИТЬ'/> </td></tr></table> </form> </details></fieldset><br/><fieldset style='width: auto'> <details> <summary>Общии настройки</summary> <br><form action='javascript:sendDateTime()'> <h5 align='left'><b>Установка дата время</b></h5> <table> <tr> <td> <h5 id='id_date'>дата время</h5></td><td> <input type='submit' name='data' value='УСТАНОВИТЬ'/> </td></tr></table> </form> <hr> <form method='POST'> <h5 align='left'><b>Время выключения</b></h5> <table> <tr> <td> <input type='checkbox' id='id_pe' name='pe'/> </td><td> <select id='id_pt' name='pt' title="Время выключения"> <option value='600000'>10мин</option> <option value='1200000'>20мин</option> <option value='1800000'>30мин</option> <option value='2400000'>40мин</option> </select> </td><td> <input type='submit' value='УСТАНОВИТЬ'/> </td></tr></table> </form> <hr> <form method='POST'> <h5>Настройки база данных интернет</h5> <table> <tr> <td>СЕРВЕР:</td><td> <input id='id_host' name='host' placeholder='сервер'> </td></tr><tr> <td>ПИН:</td><td> <input id='id_pin' name='pin' placeholder='пин весов'> </td></tr><tr> <td><a href='javascript:openSDB();'>открыть</a></td><td> <input id='id_submit_code' type='submit' value='СОХРАНИТЬ'/> </td></tr></table> </form> <hr> <form method='POST'> <h5>Доступ к настройкам</h5> <table> <tr> <td>ИМЯ:</td><td> <input id='id_n_admin' name='n_admin' placeholder='имя админ'> </td></tr><tr> <td>ПАРОЛЬ:</td><td> <input type='password' id='id_p_admin' name='p_admin' placeholder='пароль админ'> </td></tr><tr> <td/> <td> <input type='submit' value='СОХРАНИТЬ'/> </td></tr></table> </form> </details></fieldset><br/><fieldset> <details> <summary>Информация</summary> <br><span style='font-size: small; font-weight: bold'> <table> <tr> <td>Имя хоста:</td><td align='right' id='id_local_host'/> </tr></table> <hr> <h5 align='left'><b>Точка доступа весов</b></h5> <table> <tr> <td id='id_ap_ssid'/> <td align='right' id='id_ap_ip'/> </tr></table> <hr> <table> <tr> <td>MAC:</td><td align='right' id='id_mac'/> </tr></table> <hr> <table> <tr> <td>Пломба:</td><td align='right'><div id='sl_id'></div></td></tr></table> </span> <hr><a href='/calibr.html'>калибровка</a></details></fieldset><hr><footer align='center'>2018 © Powered by www.scale.in.ua</footer></body></html>)";
#else
	const char settings_html[] PROGMEM = R"(<!DOCTYPE html><html lang='en'><head> <meta charset='UTF-8'> <meta name='viewport' content='width=device-width, initial-scale=1, maximum-scale=1.0, user-scalable=no'/> <meta http-equiv="Cache-Control" content="no-cache, no-store, must-revalidate"/> <meta http-equiv="Pragma" content="no-cache"/> <title>Настройки</title> <link rel="stylesheet" type="text/css" href="global.css"> <style>input:focus{background: #FA6; outline: none;}table{width: 100%;}input, select{width: 100%; text-align: right; font-size: 18px;}input[type=submit]{width: auto;}input[type=checkbox]{width: auto;}</style> <script>window.onload=function (){load('global.css','css', function (){load('settings.js', 'js',function (){GetSettings();})});}; function load(e, t, n){let a; var d=document; if ('js'===t){a=d.createElement("script"); a.src=e; a.type='text/javascript'; a.async=!1; a.onload=function (){n()}; d.getElementsByTagName('head')[0].appendChild(a)}else if ('css'===t){a=d.createElement('link'); a.href=e; a.rel='stylesheet'; a.type='text/css'; a.async=!1; a.onload=function (){n()}; d.getElementsByTagName('head')[0].appendChild(a)}}</script></head><body style='visibility: hidden'><a href='/' class='btn btn--s btn--blue'>&lt;</a>&nbsp;&nbsp;<strong>Настройки</strong><hr><fieldset> <details> <summary>Конфигурация сети</summary> <br><h5 align='left'><b>Точка доступа WiFi роутера</b></h5> <form id='form_id' action='javascript:formNet("form_id")'> Получать IP: <input type='checkbox' id='id_auto' name='auto' onclick='enableAuthFields(this);'/> <div id='id_ip'></div><table id='id_table_net'> <tr> <td>IP:</td><td> <input id='id_lan_ip' type='text' name='lan_ip' onfocus='setOnBlur(this)'/> </td></tr><tr> <td>ШЛЮЗ:</td><td> <input id='id_gateway' type='text' name='gateway' onfocus='setOnBlur(this)'/> </td></tr><tr> <td>МАСКА:</td><td> <input id='id_subnet' type='text' name='subnet' onfocus='setOnBlur(this)'/> </td></tr></table> <table> <tr> <td>СЕТЬ:</td><td> <input id='id_ssid' name='ssid' placeholder='имя сети'> </td></tr><tr> <td>КЛЮЧ:</td><td> <input id='id_key' type='password' name='key' placeholder='пароль'> </td></tr><tr> <td/> <td> <input type='submit' value='СОХРАНИТЬ'/> </td></tr></table> </form> </details></fieldset><br/><fieldset style='width: auto'> <details> <summary>Общии настройки</summary> <br><form action='javascript:sendDateTime()'> <h5 align='left'><b>Установка дата время</b></h5> <table> <tr> <td> <h5 id='id_date'>дата время</h5></td><td> <input type='submit' name='data' value='УСТАНОВИТЬ'/> </td></tr></table> </form> <hr> <form method='POST'> <h5>Настройки база данных интернет</h5> <table> <tr> <td>СЕРВЕР:</td><td> <input id='id_host' name='host' placeholder='сервер'> </td></tr><tr> <td>ПИН:</td><td> <input id='id_pin' name='pin' placeholder='пин весов'> </td></tr><tr> <td><a href='javascript:openSDB();'>открыть</a></td><td> <input id='id_submit_code' type='submit' value='СОХРАНИТЬ'/> </td></tr></table> </form> <hr> <form method='POST'> <h5>Доступ к настройкам</h5> <table> <tr> <td>ИМЯ:</td><td> <input id='id_n_admin' name='n_admin' placeholder='имя админ'> </td></tr><tr> <td>ПАРОЛЬ:</td><td> <input type='password' id='id_p_admin' name='p_admin' placeholder='пароль админ'> </td></tr><tr> <td/> <td> <input type='submit' value='СОХРАНИТЬ'/> </td></tr></table> </form> </details></fieldset><br/><fieldset> <details> <summary>Информация</summary> <br><span style='font-size: small; font-weight: bold'> <table> <tr> <td>Имя хоста:</td><td align='right' id='id_local_host'/> </tr></table> <hr> <h5 align='left'><b>Точка доступа весов</b></h5> <table> <tr> <td id='id_ap_ssid'/> <td align='right' id='id_ap_ip'/> </tr></table> <hr> <table> <tr> <td>MAC:</td><td align='right' id='id_mac'/> </tr></table> <hr> <table> <tr> <td>Пломба:</td><td align='right'><div id='sl_id'></div></td></tr></table> </span> <hr><a href='/calibr.html'>калибровка</a></details></fieldset><hr><footer align='center'>2018 © Powered by www.scale.in.ua</footer></body></html>)";
#endif

class CoreClass : public AsyncWebHandler{
	private:
		settings_t * _settings;
	
		String _username;
		String _password;
		bool _authenticated;
	
		bool saveAuth();
		bool loadAuth();
		
	public:			
		CoreClass(const String& username, const String& password);
		~CoreClass();
		void begin();
		#if POWER_PLAN
			void powerOff();
		#endif
		char* getNameAdmin(){return _settings->scaleName;};
		char* getPassAdmin(){return _settings->scalePass;};
		char* getSSID(){return _settings->wSSID;};
		char* getLanIp(){return _settings->scaleLanIp;};
		char* getGateway(){return _settings->scaleGateway;};
		char* getPASS(){return _settings->wKey;};
		bool saveEvent(const String&, const String&);
		bool eventToServer(const String&, const String&, const String&);
		String getHash(const int, const String&, const String&, const String&);
		int getPin(){return _settings->hostPin;};
				
		
		bool isAuto(){return _settings->autoIp;};		
		virtual bool canHandle(AsyncWebServerRequest *request) override final;
		virtual void handleRequest(AsyncWebServerRequest *request) override final;
		virtual bool isRequestHandlerTrivial() override final {return false;}
		
		
};

class BatteryClass{	
	protected:
		unsigned int _charge;
		int _max;
		int _get_adc(byte times = 1);	
	public:
		BatteryClass(){};
		~BatteryClass(){};
		int fetchCharge(int);
		bool callibrated();		
		void setCharge(unsigned int ch){_charge = ch;};
		unsigned int getCharge(){return _charge;};
		void setMax(int m){_max = m;};	
		int getMax(){return _max;};
};

class BlinkClass : public Task {
	public:
	unsigned int _flash = 500;
	//unsigned int _blink = 500;
	public:
	BlinkClass() : Task(500){
		pinMode(LED, OUTPUT);
		onRun(std::bind(&BlinkClass::blinkAP,this));
	};
	void blinkSTA(){
		static unsigned char clk;
		bool led = !digitalRead(LED);
		digitalWrite(LED, led);
		if (clk < 6){
			led ? _flash = 70 : _flash = 40;
			clk++;
			}else{
			_flash = 2000;
			digitalWrite(LED, HIGH);
			clk = 0;
		}
		setInterval(_flash);
	}
	void blinkAP(){
		bool led = !digitalRead(LED);
		digitalWrite(LED, led);
		setInterval(500);
	}
};


void reconnectWifi(AsyncWebServerRequest*);
extern CoreClass * CORE;
extern BatteryClass BATTERY;
#if POWER_PLAN
	void powerOff();
	extern Task POWER;
#endif
extern BlinkClass * BLINK;

#endif //_CORE_h







