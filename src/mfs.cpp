#include "mfs.h"

void mountFS() {
    if(!LittleFS.begin()){
        Serial.println("An Error has occurred while mounting LittleFS");
    }
}

void listDir(const char *dirname) {
    Serial.printf("Listing directory: %s\n", dirname);

    Dir root = LittleFS.openDir(dirname);

    while (root.next()) {
        File file = root.openFile("r");
        Serial.print("  FILE: ");
        Serial.print(root.fileName());
        Serial.print("  SIZE: ");
        Serial.println(file.size());
        file.close();
    }
}

void readFile(const char* path, String* buf) {
    File file = LittleFS.open(path, "r");
    if (!file) {
        Serial.println("Failed to open file for reading");
        return;
    }
    for (int i=0; file.available(); i++) {
        *buf += static_cast<char>(file.read());
    }
    file.close();
}

void dumpFile(const String& path) {
    File file = LittleFS.open(path, "r");
    if (!file) {
        Serial.println("Failed to open file for reading");
        return;
    }
    for (int i=0; file.available(); i++) {
        int byte = file.read();
        // Serial.print("0x");
        Serial.print(byte < 16 ? "0" : "");
        Serial.print(byte, HEX);
        Serial.print(" ");
    }
    Serial.println("");
    file.close();
}

void writeFile(const char* path, String content) {
    File file = LittleFS.open(path, "w");
    if (!file) {
        Serial.println("Failed to open file for writing");
        return;
    }

    if (file.print(content)) {
        Serial.println("File written");
    } else {
        Serial.println("Write failed");
    }
    file.close();
}



void writeBytes(File& file, char* x, const size_t size) {
    for (unsigned int i = 0; i < size; i++) {
        file.write(x[i]);
    }
}
void writeBytes(File& file, int x) {
    writeBytes(file, reinterpret_cast<char *>(&x), sizeof(int));
}
void writeBytesShort(File& file, unsigned short x) {
    writeBytes(file, reinterpret_cast<char *>(&x), sizeof(unsigned short));
}

void writeVector(File file, vector<int>& vec) {
    auto count = static_cast<unsigned short>(vec.size());
    writeBytesShort(file, count);
    file.write(reinterpret_cast<const char*>(&(vec[0])), vec.size()*sizeof(int));
}
void writeVector(File file, vector<unsigned char>& vec) {
    auto count = static_cast<unsigned short>(vec.size());
    writeBytesShort(file, count);
    file.write(&vec[0], vec.size()*sizeof(unsigned char));
}


void writeVector2(File file, vector<vector<int>>& vec) {
    auto count = static_cast<unsigned short>(vec.size());
    writeBytesShort(file, count);
    for (vector<int> intVec : vec) {
        writeVector(file, intVec);
    }
}
void writeVector2(File file, vector<vector<unsigned char>>& vec) {
    auto count = static_cast<unsigned short>(vec.size());
    writeBytesShort(file, count);
    for (vector<unsigned char> charVec : vec) {
        writeVector(file, charVec);
    }
}

vector<int> readVector(File file) {
    unsigned short count;
    file.readBytes(reinterpret_cast<char *>(&count), sizeof(unsigned short));
    vector<int> vec(count);
    if (count > 0) {
        file.readBytes(reinterpret_cast<char*>(&vec[0]), count*sizeof(int));
    }
    return vec;
}
vector<unsigned char> readVectorChar(File file) {
    unsigned short count;
    file.readBytes(reinterpret_cast<char *>(&count), sizeof(unsigned short));
    vector<unsigned char> vec(count);
    if (count > 0) {
        file.readBytes(reinterpret_cast<char*>(&vec[0]), count*sizeof(unsigned char));
    }
    return vec;
}

vector<vector<int>> readVector2(File file) {
    unsigned short count;
    file.readBytes(reinterpret_cast<char *>(&count), sizeof(unsigned short));
    vector<vector<int>> vec = vector<vector<int>>(count);
    for (unsigned short i = 0; i < count; i++) {
        vec[i] = readVector(file);
    }
    return vec;
}

vector<vector<unsigned char>> readVector2Char(File file) {
    unsigned short count;
    file.readBytes(reinterpret_cast<char *>(&count), sizeof(unsigned short));
    vector<vector<unsigned char>> vec = vector<vector<unsigned char>>(count);
    for (unsigned short i = 0; i < count; i++) {
        vec[i] = readVectorChar(file);
    }
    return vec;
}

void writeString(File file, const String& content) {
    file.print(content);
    file.write(0);
}
void writeStringVector(File& file, vector<String>& vec) {
    auto count = static_cast<unsigned short>(vec.size());
    writeBytesShort(file, count);
    for (String& string : vec) {
        writeString(file, string);
    }
}
void writeStringVector2(File& file, vector<vector<String>>& vec) {
    auto count = static_cast<unsigned short>(vec.size());
    writeBytesShort(file, count);
    for (vector<String> str_vector : vec) {
        writeStringVector(file, str_vector);
    }
}
void writeStringVector3(File& file, vector<vector<vector<String>>>& vec) {
    auto count = static_cast<unsigned short>(vec.size());
    writeBytesShort(file, count);
    for (vector<vector<String>> str_vector2 : vec) {
        writeStringVector2(file, str_vector2);
    }
}

void readString(File file, String& data, char terminator) {
    data = "";
    char c = static_cast<char>(file.read());
    while (c != terminator) {
        data += c;
        c = static_cast<char>(file.read());
    }
}
void readStringVector(File file, vector<String>& data, char terminator) {
    unsigned short count;
    file.readBytes(reinterpret_cast<char *>(&count), sizeof(unsigned short));
    data = vector<String>(count, "");
    for (unsigned short i = 0; i < count; i++) {
        readString(file, data[i], terminator);
    }
}
void readStringVector2(File file, vector<vector<String>>& data, char terminator) {
    unsigned short count;
    file.readBytes(reinterpret_cast<char *>(&count), sizeof(unsigned short));
    data = vector<vector<String>>(count);
    for (unsigned short i = 0; i < count; i++) {
        readStringVector(file, data[i], terminator);
    }
}
void readStringVector3(File file, vector<vector<vector<String>>>& data, char terminator) {
    unsigned short count;
    file.readBytes(reinterpret_cast<char *>(&count), sizeof(unsigned short));
    data = vector<vector<vector<String>>>(count);
    for (unsigned short i = 0; i < count; i++) {
        readStringVector2(file, data[i], terminator);
    }
}



void writeMem(const String& profile, Mem* mem) {
    Serial.println("Writing mem");
    delay(1000);
    File file = LittleFS.open(profile+".mem", "w");
    if (!file) {
        Serial.println("Failed to open file for writing");
        return;
    }
    writeBytes(file, reinterpret_cast<char *>(&mem->error_us), sizeof(mem->error_us));

    writeVector(file, mem->base_message);
    writeVector(file, mem->low_ranges);
    writeVector(file, mem->high_ranges);
    writeVector(file, mem->last_options);
    // Serial.println("wrote normal vectors");

    writeStringVector(file, mem->toggle_names);
    // Serial.println("wrote toggle names");

    writeStringVector2(file, mem->field_names);
    // Serial.println("wrote field names");

    writeVector2(file, mem->toggles);
    // Serial.println("wrote toggles");

    writeStringVector3(file, mem->rules);
    // Serial.println("wrote rules");

    // write int vector vector vector
    writeBytes(file, mem->fields.size());
    for (vector<vector<unsigned char>> field : mem->fields) {
        writeVector2(file, field);
    }
    // Serial.println("wrote fields");

    Serial.println(file.size());
    file.close();
}

Mem* readMem(const String& profile) {
    File file = LittleFS.open(profile+".mem", "r");
    if (!file) {
        Serial.println("Failed to open file for writing");
        return nullptr;
    }
    Mem* mem = new Mem();
    file.readBytes(reinterpret_cast<char *>(&mem->error_us), sizeof(int));
    // Serial.println("read error_us");

    mem->base_message = readVectorChar(file);
    mem->low_ranges = readVector(file);
    mem->high_ranges = readVector(file);
    mem->last_options = readVectorChar(file);
    // Serial.println("read normal vectors");

    readStringVector(file, mem->toggle_names, '\0');
    // Serial.println("read toggle names");

    readStringVector2(file, mem->field_names, '\0');
    // Serial.println("read field names");

    mem->toggles = readVector2Char(file);
    // Serial.println("read toggles");

    readStringVector3(file, mem->rules, '\0');
    // Serial.println("read rules");

    // read int vector vector vector
    size_t count;
    file.readBytes(reinterpret_cast<char *>(&count), sizeof(int));
    mem->fields = vector<vector<vector<unsigned char>>>(count);
    for (size_t i = 0; i < count; i++) {
        mem->fields[i] = readVector2Char(file);
    }
    // Serial.println("read fields");

    file.close();
    return mem;
}

void loadMem(Mem*& mem, const String& profile) {
    if (!LittleFS.exists(profile+".mem")) {
        Serial.println("creating mem");
        mem = new Mem;
        mem->error_us = 300;
        mem->base_message = vector<unsigned char>();
        mem->low_ranges = vector<int>();
        mem->high_ranges = vector<int>();
        mem->toggle_names = vector<String>();
        mem->field_names = vector<vector<String>>();
        mem->toggles = vector<vector<unsigned char>>();
        mem->fields = vector<vector<vector<unsigned char>>>();
        mem->rules = vector<vector<vector<String>>>();
        writeMem(profile, mem);
    } else {
        Serial.print("mem already exists. reading... ");
        // dumpFile(profile+".mem");
        mem = readMem(profile);
        Serial.println("success");
    }
}


void writeSchedule(Schedules* schedules) {
    Serial.println("Writing schedules");
    File file = LittleFS.open("/schedules", "w");
    if (!file) {
        Serial.println("Failed to open file for writing");
        return;
    }
    // data
    writeBytes(file, schedules->data_schedules.size());
    for (DataSchedule schedule : schedules->data_schedules) {
        writeString(file, schedule.name);
        writeString(file, schedule.profile);

        writeStringVector(file, schedule.field_names);
        writeStringVector(file, schedule.option_names);

        writeBytes(file, reinterpret_cast<char *>(&schedule.time), sizeof(Time));
    }
    // Serial.println("Wrote data schedules");
    // toggles
    writeBytes(file, schedules->toggle_schedules.size());
    for (ToggleSchedule schedule : schedules->toggle_schedules) {
        writeString(file, schedule.name);
        writeString(file, schedule.profile);

        writeString(file, schedule.toggle_name);

        writeBytes(file, reinterpret_cast<char *>(&schedule.time), sizeof(Time));
    }
    Serial.println(file.size());
    file.close();
}

Schedules* readSchedules() {
    Schedules* schedules = new Schedules();
    File file = LittleFS.open("/schedules", "r");
    int count;

    // data
    file.readBytes(reinterpret_cast<char *>(&count), sizeof(int));
    schedules->data_schedules = vector<DataSchedule>(count);
    for (int i = 0; i < count; i++) {
        readString(file, schedules->data_schedules[i].name, '\0');
        readString(file, schedules->data_schedules[i].profile, '\0');

        readStringVector(file, schedules->data_schedules[i].field_names, '\0');
        readStringVector(file, schedules->data_schedules[i].option_names, '\0');

        file.readBytes(reinterpret_cast<char *>(&schedules->data_schedules[i].time), sizeof(Time));
    }

    // toggles
    file.readBytes(reinterpret_cast<char *>(&count), sizeof(int));
    schedules->toggle_schedules = vector<ToggleSchedule>(count);
    for (int i = 0; i < count; i++) {
        readString(file, schedules->toggle_schedules[i].name, '\0');
        readString(file, schedules->toggle_schedules[i].profile, '\0');

        readString(file, schedules->toggle_schedules[i].toggle_name, '\0');

        file.readBytes(reinterpret_cast<char *>(&schedules->toggle_schedules[i].time), sizeof(Time));
    }

    file.close();
    return schedules;
}

void loadSchedules(Schedules*& schedules) {
    if (!LittleFS.exists("/schedules")) {
        Serial.println("creating schedules");
        schedules = new Schedules;
        schedules->data_schedules = vector<DataSchedule>();
        schedules->toggle_schedules = vector<ToggleSchedule>();

        writeSchedule(schedules);
    } else {
        Serial.println("schedules already exists");
        schedules = readSchedules();
    }
}
