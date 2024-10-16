#ifndef UTILS_H
#define UTILS_H

#include <Arduino.h>
#include <vector>
#include "mfs.h"

using namespace std;

bool getMessage(vector<int>& raw_message, int sensor, int led);
void makeRanges(vector<int>& unknown_ranges, Mem* mem);
int findRange(int pulse, bool state, Mem* mem);
void processMessage(vector<int>& raw_message, vector<int>& message, Mem* mem);
void sendMessage(vector<int>& message, int pin, Mem* mem);
int findField(String field, Mem* mem);


#endif //UTILS_H
