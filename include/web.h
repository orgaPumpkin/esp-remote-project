#ifndef WEB_H
#define WEB_H

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include "utils.h"
#include "dataMessage.h"

void startWIFI(Mem* mem);
bool checkAuth(ESP8266WebServer &server);
void addAuth(String* logid);
void removeAuth(String cookie);

void ledPage(ESP8266WebServer& server,String& led_state, int led);

void loginDisconnect(ESP8266WebServer& server);
void loginConnected(ESP8266WebServer& server);
void loginLogin(ESP8266WebServer& server, Mem* mem);

void baseRecord(ESP8266WebServer& server, Mem* mem, int sensor, int led);
void baseReset(ESP8266WebServer& server, Mem* mem);

void remote(ESP8266WebServer& server, Mem* mem, const char* message);
void remoteBase(ESP8266WebServer& server, Mem* mem, int ir);
void remoteWifi(ESP8266WebServer& server, Mem* mem);
void remoteEditUser(ESP8266WebServer& server, Mem* mem);

void remoteAddToggle(ESP8266WebServer& server, Mem* mem, int sensor, int led);
void remoteRemoveToggle(ESP8266WebServer& server, Mem* mem);
void remoteAddField(ESP8266WebServer& server, Mem* mem);
void remoteRemoveField(ESP8266WebServer& server, Mem* mem);

void remoteToggle(ESP8266WebServer& server, Mem* mem, int ir);
void remoteSendData(ESP8266WebServer& server, Mem* mem, int ir);

void editField(ESP8266WebServer& server, Mem* mem, String message);
void editFieldAddOption(ESP8266WebServer& server, Mem* mem, int sensor, int led);
void editFieldRemoveOption(ESP8266WebServer& server, Mem* mem);
void editFieldEditRule(ESP8266WebServer& server, Mem* mem);


#endif //WEB_H
