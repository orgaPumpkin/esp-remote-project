#ifndef MFS_H
#define MFS_H

#include <FS.h>
#include <LittleFS.h>
#include <vector>


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

void mountFS();
void listDir(const char *dirname);
void readFile(const char* path, String* buf);
void dumpFile(const char* path);
void writeFile(const char *path, String content);
void writeVector(File file, vector<int>* vec);
vector<int> readVector(File file);
void readString(File file, String& data, char terminator);
void writeMem(String path, Mem* mem);
Mem* readMem(String path);
void loadMem(Mem*& mem, String path);

#endif //MFS_H
