#ifndef DATAMESSAGE_H
#define DATAMESSAGE_H

#include <vector>
#include <ESP8266WebServer.h>
#include <mfs.h>
#include <utils.h>

using std::vector;

typedef struct fieldValue {
    unsigned int fieldI;
    unsigned int optionI;
    vector<bool> effected;
} fieldValue;

vector<bool> findEffected(unsigned int fieldI, Mem* mem);
vector<fieldValue> getFieldsServer(ESP8266WebServer& server, Mem* mem);
vector<fieldValue> getFieldsSchedule(DataSchedule& schedule, Mem* mem);
vector<unsigned char> buildDataMessage(vector<fieldValue> fields, Mem* mem);


#endif //DATAMESSAGE_H
