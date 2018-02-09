// Updater.h

#ifndef _HTTPUPDATER_h
#define _HTTPUPDATER_h
#include "BrowserServer.h"

static const char successResponse[] PROGMEM = "<META http-equiv=\"refresh\" content=\"15;URL=/\">Обновление успешно! Перегрузка...\n";

class BrowserServerClass;

class HttpUpdaterClass{
	public:
		HttpUpdaterClass();

		void setup(BrowserServerClass *server){
			setup(server, NULL, NULL);
		}

		void setup(BrowserServerClass *server, const char * path){
			setup(server, path, NULL, NULL);
		}

		void setup(BrowserServerClass *server, const char * username, const char * password){
			setup(server, "/update", username, password);
		}

		void setup(BrowserServerClass *server, const char * path, const char * username, const char * password);

	protected:
		void _setUpdaterError();

	private:
		//bool _serial_output;
		BrowserServerClass *_server;
		char * _username;
		char * _password;
		bool _authenticated;
		String _updaterError;
		int _command;
};

extern HttpUpdaterClass httpUpdater;

#endif //_HTTPUPDATER_h

