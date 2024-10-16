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
    Serial.printf("Reading file: %s\n", path);

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

void dumpFile(const char* path) {
    Serial.printf("Dumping file: %s\n", path);

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
    Serial.printf("Writing file: %s\n", path);

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

void writeInt(File file, int x) {
    char* intc = reinterpret_cast<char *>(&x);
    for (unsigned int i = 0; i < sizeof(int); i++) {
        file.write(intc[i]);
    }
}

void writeVector(File file, vector<int>* vec) {
    int count = vec->size();
    writeInt(file, count);
    if (count > 0) {
        file.write(reinterpret_cast<const char*>(&((*vec)[0])), vec->size()*sizeof(int));
    }
}

vector<int> readVector(File file) {
    size_t count;
    file.readBytes(reinterpret_cast<char *>(&count), sizeof(int));
    vector<int> vec(count);
    if (count > 0) {
        file.readBytes(reinterpret_cast<char*>(&vec[0]), count*sizeof(int));
    }
    return vec;
}

void writeString(File file, String content) {
    file.print(content);
    file.write(0);
}

void readString(File file, String* data) {
    char c = static_cast<char>(file.read());
    while (c) {
        *data += c;
        c = static_cast<char>(file.read());
    }
}


void writeMem(const char* path, Mem* mem) {
    Serial.println("Writing mem");
    delay(1000);
    File file = LittleFS.open(path, "w");
    if (!file) {
        Serial.println("Failed to open file for writing");
        return;
    }
    writeInt(file, mem->error_us);

    writeString(file, mem->wifi_ssid);
    writeString(file, mem->wifi_pass);
    writeString(file, mem->username);
    writeString(file, mem->password);
    // Serial.println("wrote strings");

    writeVector(file, &mem->base_message);
    writeVector(file, &mem->low_ranges);
    writeVector(file, &mem->high_ranges);
    writeVector(file, &mem->high_ranges);
    // Serial.println("wrote normal vectors");

    // write string vector
    writeInt(file, mem->toggle_names.size());
    for (String toggle_name : mem->toggle_names) {
        writeString(file, toggle_name);
    }
    // Serial.println("wrote toggle names");

    // write string vector vector
    writeInt(file, mem->field_names.size());
    for (vector<String> str_vector : mem->field_names) {
        writeInt(file, str_vector.size());
        for (String str : str_vector) {
            writeString(file, str);
        }
    }
    // Serial.println("wrote field names");

    // write int vector vector
    writeInt(file, mem->toggles.size());
    for (vector<int> toggle : mem->toggles) {
        writeVector(file, &toggle);
    }
    // Serial.println("wrote toggles");

    // write string vector vector vector
    writeInt(file, mem->rules.size());
    for (vector<vector<String>> field : mem->rules) {
        writeInt(file, field.size());
        for (vector<String> option : field) {
            writeInt(file, option.size());
            for (String disabled_field : option) {
                writeString(file, disabled_field);
            }
        }
    }
    // Serial.println("wrote rules");

    // write int vector vector vector
    writeInt(file, mem->fields.size());
    for (vector<vector<int>> field : mem->fields) {
        writeInt(file, field.size());
        for (vector<int> option : field) {
            writeVector(file, &option);
        }
    }
    // Serial.println("wrote fields");

    file.close();
}

Mem* readMem(const char* path) {
    File file = LittleFS.open(path, "r");
    if (!file) {
        Serial.println("Failed to open file for writing");
        return nullptr;
    }
    Mem* mem = new Mem();
    file.readBytes(reinterpret_cast<char *>(&mem->error_us), sizeof(int));
    // Serial.println("read error_us");

    mem->wifi_ssid = "";
    readString(file, &mem->wifi_ssid);
    mem->wifi_pass = "";
    readString(file, &mem->wifi_pass);
    mem->username = "";
    readString(file, &mem->username);
    mem->password = "";
    readString(file, &mem->password);

    // Serial.println("read strings");
    mem->base_message = readVector(file);
    mem->low_ranges = readVector(file);
    mem->high_ranges = readVector(file);
    mem->last_options = readVector(file);
    // Serial.println("read normal vectors");
    // read string vector
    size_t count;
    file.readBytes(reinterpret_cast<char *>(&count), sizeof(int));
    mem->toggle_names = vector<String>(count, "");
    for (size_t i = 0; i < count; i++) {
        char c = static_cast<char>(file.read());
        while (c) {
            mem->toggle_names[i] += c;
            c = static_cast<char>(file.read());
        }
    }
    // Serial.println("read toggle names");
    // read string vector vector
    size_t count2;
    file.readBytes(reinterpret_cast<char *>(&count), sizeof(int));
    mem->field_names = vector<vector<String>>(count);
    for (size_t i = 0; i < count; i++) {
        file.readBytes(reinterpret_cast<char *>(&count2), sizeof(int));
        mem->field_names[i] = vector<String>(count2, "");
        for (size_t j = 0; j < count2; j++) {
            char c = static_cast<char>(file.read());
            while (c) {
                mem->field_names[i][j] += c;
                c = static_cast<char>(file.read());
            }
        }
    }
    // Serial.println("read field names");
    // read int vector vector
    file.readBytes(reinterpret_cast<char *>(&count), sizeof(int));
    mem->toggles = vector<vector<int>>(count);
    for (size_t i = 0; i < count; i++) {
        mem->toggles[i] = readVector(file);
    }
    // Serial.println("read toggles");

    // read int vector vector vector
    size_t count3;
    file.readBytes(reinterpret_cast<char *>(&count), sizeof(int));
    mem->rules = vector<vector<vector<String>>>(count);
    for (size_t i = 0; i < count; i++) {
        file.readBytes(reinterpret_cast<char *>(&count2), sizeof(int));
        mem->rules[i] = vector<vector<String>>(count2);
        for (size_t j = 0; j < count2; j++) {
            file.readBytes(reinterpret_cast<char *>(&count3), sizeof(int));
            mem->rules[i][j] = vector<String>(count3, "");
            for (size_t k = 0; k < count3; k++) {
                char c = static_cast<char>(file.read());
                while (c) {
                    mem->rules[i][j][k] += c;
                    c = static_cast<char>(file.read());
                }
            }
        }
    }
    // Serial.println("read rules");

    // read int vector vector vector
    file.readBytes(reinterpret_cast<char *>(&count), sizeof(int));
    mem->fields = vector<vector<vector<int>>>(count);
    for (size_t i = 0; i < count; i++) {
        file.readBytes(reinterpret_cast<char *>(&count2), sizeof(int));
        mem->fields[i] = vector<vector<int>>(count2);
        for (size_t j = 0; j < count2; j++) {
            mem->fields[i][j] = readVector(file);
        }
    }
    // Serial.println("read fields");

    file.close();
    return mem;
}