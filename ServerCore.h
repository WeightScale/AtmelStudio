/* 
* ServerCoreClass.h
*
* Created: 20.04.2018 13:40:19
* Author: Kostya
*/
#ifndef __SERVERCORECLASS_H__
#define __SERVERCORECLASS_H__
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>

#define AUTH_FILE "/secret.json"

class ServerCoreClass : public AsyncWebServer{
//variables
public:
protected:
	bool _saveHTTPAuth();
	bool _downloadHTTPAuth();
private:
	strHTTPAuth _httpAuth;

//functions
public:
	ServerCoreClass(uint16_t port);
	~ServerCoreClass();
	void init();
protected:
private:
	ServerCoreClass( const ServerCoreClass &c );
	ServerCoreClass& operator=( const ServerCoreClass &c );

}; //ServerCoreClass

extern ServerCoreClass ServerCore;

#endif //__SERVERCORECLASS_H__
