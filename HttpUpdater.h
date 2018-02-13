// Updater.h

#ifndef _HTTPUPDATER_h
#define _HTTPUPDATER_h
#include "BrowserServer.h"

static const char successResponse[] PROGMEM = "<META http-equiv=\"refresh\" content=\"15;URL=/\">Обновление успешно! Перегрузка...\n";

class BrowserServerClass;

class HttpUpdaterClass{
	public:
		HttpUpdaterClass();
		
		void setup(BrowserServerClass *server, const char * username, const char * password){
			setup(server, "/update", username, password);
		}

		void setup(BrowserServerClass *server, const char * path, const char * username, const char * password);
		
		BrowserServerClass *getServer(){ return _server;};
		char * getUserName(){return _username;};
		char * getPassword(){return _password;};
		void setAuthenticated(bool a){_authenticated = a;};
		bool getAuthenticated(){return _authenticated;};

	protected:
		

	private:		
		BrowserServerClass *_server;
		char * _username;
		char * _password;
		bool _authenticated;
		
		
};

extern HttpUpdaterClass httpUpdater;

void handleUpdatePage();
void handleStartUpdate();
void handleEndUpdate();
void setUpdaterError();
void handleHttpStartUpdate();

#endif //_HTTPUPDATER_h

