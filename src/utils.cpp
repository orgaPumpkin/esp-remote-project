#include "utils.h"
#include <dataMessage.h>

bool getMessage(vector<int>& raw_message, int sensor, int led) {
    bool state = LOW;

    digitalWrite(led, LOW);
    unsigned long st = millis();
    while (digitalRead(sensor) == HIGH) { // while message didn't start
        if (millis() - st > 5*1000) {
            digitalWrite(led, HIGH);
            return false;
        }
        yield();
    }

    st = micros();
    while (micros() - st <= 100000) {
        if (digitalRead(sensor) == !state) {
            raw_message.push_back(micros() - st);
            st = micros();
            state = !state;
        }
    }
    digitalWrite(led, HIGH);
    return true;
}

void makeRanges(vector<int>& unknown_ranges, vector<int> &result, Mem* mem) {
    sort(unknown_ranges.begin(), unknown_ranges.end());
    int minRange = unknown_ranges[0];
    int maxRange = minRange;
    int rangeLimit = minRange + 2*mem->error_us;

    for (int pulse : unknown_ranges) {
        if (pulse <= rangeLimit) {
            maxRange = pulse;
        } else {
            result.push_back((minRange+maxRange)/2);
            minRange = pulse;
            maxRange = minRange;
            rangeLimit = minRange + 2*mem->error_us;
        }
    }
    result.push_back((minRange+maxRange)/2);
}

short findRange(int pulse, bool state, Mem *mem) {
    short closest = -1;
    int distance = -1;
    vector<int>* ranges;
    if (state) {
        ranges = &mem->high_ranges;
    } else {
        ranges = &mem->low_ranges;
    }
    for (unsigned int i = 0; i < ranges->size(); i++) {
        if (abs(pulse - ranges->at(i)) <= abs(mem->error_us)) {  // close enough
            if (abs(pulse - ranges->at(i)) < distance or distance == -1) {  // closer
                closest = i;
                distance = abs(pulse - ranges->at(i));
            }
        }
    }
    return closest;
}

void processMessage(vector<int>& raw_message, vector<unsigned char> &message, Mem* mem) {
    vector<int> low_unknowns;
    vector<int> high_unknowns;
    bool state = LOW;

    // find unknowns
    for (int pulse : raw_message) {
        short range = findRange(pulse, state, mem);
        if (range == -1) {
            if (state) {
                high_unknowns.push_back(pulse);
            } else {
                low_unknowns.push_back(pulse);
            }
        }
        state = !state;
    }
    // make ranges
    if (!high_unknowns.empty()) { makeRanges(high_unknowns, mem->high_ranges, mem);}
    if (!low_unknowns.empty()) { makeRanges(low_unknowns, mem->low_ranges, mem);}

    // assign ranges
    message = vector<unsigned char>();
    state = LOW;
    for (int pulse : raw_message) {
        if (findRange(pulse, state, mem) == -1) {Serial.println("Message proccessing error!!!");}
        message.push_back(static_cast<unsigned char>(findRange(pulse, state, mem)));
        state = !state;
    }
}

void sendMessage(vector<unsigned char> &message, int pin, Mem* mem) {
    unsigned long st;
    bool state = LOW;
    int pulse;
    for (unsigned char range : message) {
        if (!state) {
            pulse = mem->low_ranges[range];
            st = micros();
            while (static_cast<int>(micros() - st) < pulse) {
                digitalWrite(pin, HIGH);
                delayMicroseconds(13);
                digitalWrite(pin, LOW);
                delayMicroseconds(13);
            }
        } else {
            pulse = mem->high_ranges[range];
            delayMicroseconds(pulse);
        }
        state = !state;
    }
    Serial.println("sent message");
}

int findField(String field, Mem* mem) {
    for (unsigned int i = 0; i < mem->field_names.size(); i++) {
        if (mem->field_names[i][0] == field) {
            return i;
        }
    }
    return -1;
}

unsigned int findElement(const String &option, vector<String> &vec) {
    auto optionPtr = find(vec.begin(), vec.end(), option);
    return optionPtr - vec.begin();
}

String vectorToString(vector<String> &vec) {
    String result = "";
    for (String& i : vec) {
        result += i +",";
    }
    if (result.length() > 0) {
        result.remove(result.length() - 1);
    }
    return result;
}

String vectorToString(vector<int> &vec) {
    String result = "";
    for (int i : vec) {
        result += String(i) +",";
    }
    if (result.length() > 0) {
        result.remove(result.length() - 1);
    }
    return result;
}

bool getProfile(const String& profile, String& curr_profile, vector<String>& profiles, Mem*& mem) {
    if (count(profiles.begin(), profiles.end(), profile) > 0) {
        if (profile != curr_profile) {
            curr_profile = profile;
            delete mem;
            loadMem(mem, profile);
        }
    } else {
        return false;
    }
    return true;
}

void sendSchedules(Schedules* schedules, NTPClient& timeClient, int ir_pin) {
    // data
    Serial.println(timeClient.getFormattedTime());
    for (DataSchedule schedule : schedules->data_schedules) {
        if (schedule.time.minute == timeClient.getMinutes() && schedule.time.hour == timeClient.getHours() && schedule.time.days[timeClient.getDay()]) {
            Serial.println("sending schedule: " + schedule.name);
            Mem* mem;
            loadMem(mem, schedule.profile);

            vector<fieldValue> fields = getFieldsSchedule(schedule, mem);
            vector<unsigned char> message = buildDataMessage(fields, mem);
            sendMessage(message, ir_pin, mem);
            delete mem;
            delay(100);
        }
    }
    // toggle
    for (ToggleSchedule schedule : schedules->toggle_schedules) {
        if (schedule.time.minute == timeClient.getMinutes() && schedule.time.hour == timeClient.getHours() && schedule.time.days[timeClient.getDay()]) {
            Serial.println("sending schedule: " + schedule.name);
            Mem* mem;
            loadMem(mem, schedule.profile);

            unsigned int toggleI = findElement(schedule.toggle_name, mem->toggle_names);
            if (toggleI < mem->toggle_names.size()) {
                vector<unsigned char> message = mem->toggles[toggleI];
                sendMessage(message, ir_pin, mem);

                delay(100);
            }
            delete mem;
        }
    }
}
