#ifndef MFS_H
#define MFS_H

#include <FS.h>
#include <LittleFS.h>
#include <vector>
#include <TimeLib.h>


using namespace std;

typedef struct Mem {
    int error_us;
    vector<int> base_message;
    vector<int> low_ranges;
    vector<int> high_ranges;
    vector<int> last_options;
    vector<String> toggle_names;
    vector<vector<String>> field_names;
    vector<vector<int>> toggles;
    vector<vector<vector<String>>> rules;
    vector<vector<vector<int>>> fields;
} Mem;


typedef struct DataSchedule {
    String name;
    vector<String> field_names;
    vector<String> option_names;
    time_t time;
} DataSchedule;

typedef struct ToggleSchedule {
    String name;
    String toggle_name;
    time_t time;
} Toggle;

typedef struct Schedules {
    vector<DataSchedule> data_schedules;
    vector<ToggleSchedule> toggle_schedules;
} Schedules;

void mountFS();
void listDir(const char *dirname);
void readFile(const char* path, String* buf);
void dumpFile(const char* path);
void writeFile(const char *path, String content);

void readString(File file, String& data, char terminator);

void writeMem(const String& profile, Mem* mem);
void loadMem(Mem*& mem, const String& profile);

void writeSchedule(Schedules* schedules, const String& profile);
void loadSchedules(Schedules*& schedules, const String& profile);


#endif //MFS_H
