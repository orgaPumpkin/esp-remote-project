#ifndef UTILS_H
#define UTILS_H

#include <Arduino.h>
#include <NTPClient.h>
#include <vector>
#include "mfs.h"

using std::vector;

bool getMessage(vector<int>& raw_message, int sensor, int led);
void makeRanges(vector<int>& unknown_ranges, Mem* mem);
int findRange(int pulse, bool state, Mem* mem);
void processMessage(vector<int>& raw_message, vector<int>& message, Mem* mem);
void sendMessage(vector<int>& message, int pin, Mem* mem);
int findField(String field, Mem* mem);
unsigned int findElement(const String &option, vector<String> &vec);
bool getProfile(const String& profile, String& curr_profile, vector<String>& profiles, Mem*& mem);
void sendSchedules(Schedules* schedules, NTPClient& timeClient, int ir_pin);


#endif //UTILS_H
