#ifndef WEB_H
#define WEB_H

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <NTPClient.h>

#include "utils.h"
#include "dataMessage.h"

void startWIFI(String& ssid, String& pass);
bool checkAuth(ESP8266WebServer &server);
void addAuth(String* logid);
void removeAuth(String cookie);


void ledPage(ESP8266WebServer& server,String& led_state, int led);


void loginDisconnect(ESP8266WebServer& server);
void loginConnected(ESP8266WebServer& server);
void loginLogin(ESP8266WebServer& server, String username, String password);


void rootShow(ESP8266WebServer& server, vector<String>& profiles);
void rootRemove(ESP8266WebServer& server, vector<String>& profiles);
void rootAdd(ESP8266WebServer& server, vector<String>& profiles);


void remoteShow(ESP8266WebServer& server, Mem* mem, const String& message);

void remoteToggle(ESP8266WebServer& server, Mem* mem, int ir);
void remoteSendData(ESP8266WebServer& server, Mem* mem, int ir);


void editShow(ESP8266WebServer& server, Mem* mem, const String& message);

void editAddToggle(ESP8266WebServer& server, Mem* mem, int sensor, int led);
void editRemoveToggle(ESP8266WebServer& server, Mem* mem);
void editAddField(ESP8266WebServer& server, Mem* mem);
void editRemoveField(ESP8266WebServer& server, Mem* mem);

void editSendBase(ESP8266WebServer& server, Mem* mem, int ir);
void editRecordBase(ESP8266WebServer& server, Mem* mem, int sensor, int led);
void editReset(ESP8266WebServer& server, Mem* mem, Schedules* schedules);


void setupShow(ESP8266WebServer& server, const String& message);
void setupWifi(ESP8266WebServer& server, String& ssid, String& pass);
void setupUser(ESP8266WebServer& server, String& username, String& password);
void setupTimeZone(ESP8266WebServer& server, NTPClient& ntp);


void editField(ESP8266WebServer& server, Mem* mem, const String& message);
void editFieldAddOption(ESP8266WebServer& server, Mem* mem, int sensor, int led);
void editFieldRemoveOption(ESP8266WebServer& server, Mem* mem);
void editFieldEditRule(ESP8266WebServer& server, Mem* mem);


void schedulesShow(ESP8266WebServer& server, Schedules* schedules, vector<String>& profiles, const String& message);
void schedulesAdd(ESP8266WebServer& server, vector<String>& profiles, Schedules* schedules);
void schedulesRemove(ESP8266WebServer& server, vector<String>& profiles, Schedules* schedules);


void editScheduleShow(ESP8266WebServer& server, Schedules* schedules, const String& message);
void editSchedulesEdit(ESP8266WebServer& server, Schedules* schedules);


#endif //WEB_H
