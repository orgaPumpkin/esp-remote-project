#ifndef DATAMESSAGE_H
#define DATAMESSAGE_H

#include <vector>
#include <ESP8266WebServer.h>
#include <mfs.h>
#include <utils.h>

using namespace std;

vector<bool> findEffected(unsigned int fieldI, Mem* mem);
vector<int> buildDataMessage(ESP8266WebServer& server, Mem* mem);


#endif //DATAMESSAGE_H
