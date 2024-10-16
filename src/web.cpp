#include "web.h"
#include "mfs.h"

void startWIFI(Mem* mem) {
    Serial.println(mem->wifi_ssid);
    if (mem->wifi_ssid != "") {
        // connect to WiFi
        Serial.print("Connecting to ");
        Serial.println(mem->wifi_ssid);
        WiFi.begin(mem->wifi_ssid, mem->wifi_pass);
        unsigned long st = millis();
        while (WiFi.status() != WL_CONNECTED && millis()-st < 1000*30) {  //Wait for the Wi-Fi connection completion
            delay(500);
            Serial.println("Waiting for connection...");
        }
        if (WiFi.status() == WL_CONNECTED) {
            // Print local IP address and start web server
            Serial.print("Connected! IP address: ");
            Serial.println(WiFi.localIP());
            return;
        }
    }
    // unable to connect to WiFi
    Serial.print("Setting soft-AP ... ");
    boolean result = WiFi.softAP("ESP-remote");
    if(result == true) {
        Serial.println("Ready");
        Serial.println(WiFi.softAPIP());
    }
    else {
        Serial.println("Failed!");
    }
}

bool checkAuth(ESP8266WebServer& server) {
    if (server.hasHeader("Cookie")) {
        String cookie = server.header("Cookie");
        int logidI = cookie.indexOf("logid=");
        if (logidI != -1) {
            String logid = String(&cookie[logidI+6]);
            String logids = "";
            readFile("/logids.txt", &logids);
            return logids.indexOf("," + logid + ",") != -1;
        }
    }
    return false;
}

void addAuth(String* logid) {
    String logids = "";
    readFile("/logids.txt", &logids);
    *logid = String(random(99999999));
    logids += *logid + ",";
    writeFile("/logids.txt", logids);
}

void removeAuth(String cookie) {
    int logidI = cookie.indexOf("logid=");
    if (logidI != -1) {
        String logid = String(&cookie[logidI+6]);
        String logids = "";
        readFile("/logids.txt", &logids);
        logids.replace((logid) + ",", "");
        writeFile("/logids.txt", logids);
    }
}

//led
void ledPage(ESP8266WebServer& server,String& led_state, int led) {
    if (server.method() == HTTP_POST && server.hasArg("led")) {
        if (server.arg("led") == "on") {
            led_state = "on";
            digitalWrite(led, LOW);
        } else if (server.arg("led") == "off") {
            led_state = "off";
            digitalWrite(led, HIGH);
        }
    }
    String html = "";
    readFile("/led.html", &html);
    html.replace("%s", led_state);
    server.send(200, "text/html", html);
}

// login
void loginDisconnect(ESP8266WebServer& server) {
    removeAuth(server.header("Cookie"));
    server.sendHeader("Location", "/login");
    server.sendHeader("Set-Cookie", "logid=deleted; expires=Thu, 01 Jan 1970 00:00:00 GMT");
    Serial.println("Logged out");
    server.send(302);
}

void loginConnected(ESP8266WebServer& server) {
    server.sendHeader("Location", "/");
    server.send(302);
}

void loginLogin(ESP8266WebServer& server, Mem* mem) {
    if (server.arg("username") == mem->username && server.arg("password") == mem->password) { // right login
        String logid = "";
        addAuth(&logid);
        server.sendHeader("Location", "/remote");
        server.sendHeader("Cache-Control", "no-cache");
        server.sendHeader("Set-Cookie", "logid=" + logid + "; SameSite=Lax");
        server.send(302);
        Serial.println("Logged in");
    } else {    // wrong login
        String html = "";
        readFile("/login.html", &html);
        html.replace("%s", "Wrong username/password! try again.");
        server.send(200, "text/html", html);
    }
}

// base
void baseRecord(ESP8266WebServer& server, Mem* mem, int sensor, int led) {
    String html = "";
    readFile("/base.html", &html);
    vector<int> raw_message = vector<int>();
    if (getMessage(raw_message, sensor, led)) {

        mem->error_us = 0;
        for (int pulse : raw_message) {
            mem->error_us += pulse;
        }
        mem->error_us = mem->error_us / raw_message.size()/100 * 40;
        Serial.println(mem->error_us);

        vector<int> message = vector<int>();
        processMessage(raw_message, message, mem);
        for (int pulse : message) {
            Serial.print(pulse);
        }
        Serial.println("");
        mem->base_message = message;
        writeMem("/mem", mem);

        html.replace("%s", "Length: "+String(raw_message.size()));
        server.send(200, "text/html", html);
    } else {
        html.replace("%s", "no message was detected. try again.");
        server.send(200, "text/html", html);
    }
}

void baseReset(ESP8266WebServer& server, Mem* mem) {
    mem->low_ranges.clear();
    mem->high_ranges.clear();
    mem->base_message.clear();
    mem->field_names.clear();
    mem->fields.clear();
    mem->toggle_names.clear();
    mem->toggles.clear();
    mem->error_us = 300;
    mem->rules.clear();
    writeMem("/mem", mem);

    String html = "";
    readFile("/base.html", &html);
    html.replace("%s", "Remote reset");
    server.send(200, "text/html", html);
}

// remote
void remote(ESP8266WebServer& server, Mem* mem, const char* message) {
    String html = "";
    readFile("/remote.html", &html);
    html.replace("%s", message);

    String formatStr = "";
    for (vector<String> option_names : mem->field_names) {
        for (String option_name : option_names) {
            formatStr += option_name + ",";
        }
        formatStr[formatStr.length()-1] = ';';
    }
    if (formatStr.length() >= 1) {
        formatStr.remove(formatStr.length() - 1);
    }
    Serial.println(formatStr);
    html.replace("{fields}", formatStr);

    formatStr = "";
    for (String& toggle_name : mem->toggle_names) {
        formatStr += toggle_name + ",";
    }
    if (formatStr.length() >= 1) {
        formatStr.remove(formatStr.length() - 1);
    }
    Serial.println(formatStr);
    html.replace("{toggles}", formatStr);

    formatStr = "";
    for (int option : mem->last_options) {
        formatStr += String(option) + ",";
    }
    if (formatStr.length() >= 1) {
        formatStr.remove(formatStr.length() - 1);
    }
    Serial.println(formatStr);
    html.replace("{last}", formatStr);

    html.trim();
    server.send(200, "text/html", html);
}

void remoteBase(ESP8266WebServer& server, Mem* mem, int ir) {
    sendMessage(mem->base_message, ir, mem);

    remote(server, mem, "sent base data message.");
}

void remoteWifi(ESP8266WebServer& server, Mem* mem) {
    mem->wifi_ssid = server.arg("wifi_ssid");
    mem->wifi_pass = server.arg("wifi_pass");
    writeMem("/mem", mem);
    remote(server, mem, "WiFi details updated, restart esp to apply changes.");
}

void remoteEditUser(ESP8266WebServer& server, Mem* mem) {
    mem->username = server.arg("username");
    mem->password = server.arg("password");
    writeMem("/mem", mem);
    remote(server, mem, "login details updated.");
}

void remoteAddToggle(ESP8266WebServer& server, Mem* mem, int sensor, int led) {
    // get message
    vector<int> raw_message = vector<int>();
    if (getMessage(raw_message, sensor, led)) {  // if got a message
        unsigned int  toggleI = distance(mem->toggle_names.begin(), find(mem->toggle_names.begin(), mem->toggle_names.end(), server.arg("add_toggle")));
        Serial.println("toggleI = " + String(toggleI));
        // if toggle doesn't already exist, create
        if (toggleI >= mem->toggle_names.size()) {
            mem->toggle_names.push_back(server.arg("add_toggle"));
            mem->toggles.emplace_back();
            Serial.println("added toggle");
            toggleI = mem->toggle_names.size() - 1;
        }
        // save message
        processMessage(raw_message, mem->toggles[toggleI], mem);
        Serial.println("edited toggle");
        writeMem("/mem", mem);

        remote(server, mem, "constant was recorded.");
    } else {
        remote(server, mem, "no message was detected. try again.");
    }
}

void remoteRemoveToggle(ESP8266WebServer& server, Mem* mem) {
    unsigned int  toggleI = distance(mem->toggle_names.begin(), find(mem->toggle_names.begin(), mem->toggle_names.end(), server.arg("remove_toggle")));
    if (toggleI < mem->toggle_names.size()) { // if toggle exists
        // remove toggle
        mem->toggle_names.erase(mem->toggle_names.begin() + toggleI);
        mem->toggles.erase(mem->toggles.begin() + toggleI);
        Serial.println("removed toggle");
        writeMem("/mem", mem);
        remote(server, mem, "constant was removed.");
    } else {
        remote(server, mem, "constant not found");
    }
}

void remoteAddField(ESP8266WebServer& server, Mem* mem) {
    int fieldI = findField(server.arg("add_field"), mem);
    // if field doesn't already exist, create
    if (fieldI == -1) {
        mem->field_names.emplace_back();
        mem->field_names[mem->field_names.size()-1].push_back(server.arg("add_field"));
        mem->fields.emplace_back();
        mem->last_options.emplace_back();
        mem->rules.emplace_back();
        writeMem("/mem", mem);
        Serial.println("added field");
    }
    server.sendHeader("Location", "/edit_field?field="+server.arg("add_field"));
    server.send(302);
}

void remoteRemoveField(ESP8266WebServer& server, Mem* mem) {

    int fieldI = findField(server.arg("remove_field"), mem);
    if (fieldI != -1) { // if field exists
        // remove toggle
        mem->field_names.erase(mem->field_names.begin() + fieldI);
        mem->fields.erase(mem->fields.begin() + fieldI);
        mem->last_options.erase(mem->last_options.begin() + fieldI);
        mem->rules.erase(mem->rules.begin() + fieldI);
        Serial.println("removed field");
        writeMem("/mem", mem);
        remote(server, mem, "field was removed.");
    } else {
        remote(server, mem, "field not found");
    }
}

void remoteToggle(ESP8266WebServer& server, Mem* mem, int ir) {
    unsigned int  toggleI = distance(mem->toggle_names.begin(), find(mem->toggle_names.begin(), mem->toggle_names.end(), server.arg("toggle")));

    if (toggleI < mem->toggle_names.size()) {   // toggle exists
        sendMessage(mem->toggles[toggleI], ir, mem);
        remote(server, mem, "sent toggle.");
    } else {
        remote(server, mem, "invalid toggle");
    }
}

void remoteSendData(ESP8266WebServer& server, Mem* mem, int ir) {
    vector<int> message = buildDataMessage(server, mem);
    for (int pulse : message) {
        Serial.print(pulse);
    }
    Serial.println("");
    sendMessage(message, ir, mem);
    remote(server, mem, "sent data message.");
}

// edit field
void editField(ESP8266WebServer& server, Mem* mem, String message) {
    String html = "";
    readFile("/edit_field.html", &html);
    html.replace("{field}", server.arg("field"));

    String optionsStr = "";
    int fieldI = findField(server.arg("field"), mem);
    for (unsigned int i = 1; i < mem->field_names[fieldI].size(); i++) {
        optionsStr += mem->field_names[fieldI][i] + ",";
    }
    if (optionsStr.length() >= 1) {
        optionsStr.remove(optionsStr.length() - 1);
    }
    html.replace("{options}", optionsStr);

    if (mem->fields[fieldI].size() > 0) {
        Serial.println("getting disable_fields");
        String disable_fields = "";
        vector<bool> effected = findEffected(fieldI, mem);
        int effected_count = count(effected.begin(), effected.end(), true);
        Serial.println("found effected");
        for (unsigned int i = 0; i < mem->fields.size(); i++) {
            if (i != static_cast<unsigned>(fieldI)) {
                Serial.print("field: " + String(i));
                vector<bool> field_effected = findEffected(i, mem);
                int field_effected_count = count(field_effected.begin(), field_effected.end(), true);
                for (unsigned int j = 0; j < effected.size(); j++) {
                    if (effected[j] && field_effected[j] && field_effected_count < effected_count) {
                        disable_fields += mem->field_names[i][0] + ",";
                        break;
                    }
                }
            }
        }
        if (disable_fields.length() >= 1) {
            disable_fields.remove(disable_fields.length() - 1);
        }
        html.replace("{disable_fields}", disable_fields);
    } else {
        html.replace("{disable_fields}", "");
    }

    html.replace("%s", message);

    server.send(200, "text/html", html);
}

void editFieldAddOption(ESP8266WebServer& server, Mem* mem, int sensor, int led) {
    // get message
    vector<int> raw_message = vector<int>();
    if (getMessage(raw_message, sensor, led)) {  // if got a message
        if (raw_message.size() == mem->base_message.size()) {
            int fieldI = findField(server.arg("field"), mem);
            unsigned int optionI = distance(mem->field_names[fieldI].begin(), find(++mem->field_names[fieldI].begin(), mem->field_names[fieldI].end(), server.arg("add_option")))-1;
            Serial.println("optionI = " + String(optionI));
            // if option doesn't already exist, create
            if (optionI >= mem->field_names[fieldI].size()-1) {
                mem->field_names[fieldI].push_back(server.arg("add_option"));
                mem->fields[fieldI].emplace_back();
                mem->rules[fieldI].emplace_back();
                Serial.println("added option");
                optionI = mem->field_names[fieldI].size() - 2;
            }
            // save message
            processMessage(raw_message, mem->fields[fieldI][optionI], mem);
            Serial.println("edited option");
            writeMem("/mem", mem);

            editField(server, mem, "option was recorded successfully.");
        } else {
            editField(server, mem, "recorded message doesn't match base message.");
        }
    } else {
        editField(server, mem, "no message was detected. try again.");
    }
    editField(server, mem, "no message was recorded.");
}

void editFieldRemoveOption(ESP8266WebServer& server, Mem* mem) {

    int fieldI = findField(server.arg("field"), mem);
    Serial.println(server.arg("remove_option"));
    unsigned int optionI = (find(mem->field_names[fieldI].begin(), mem->field_names[fieldI].end(), server.arg("remove_option")) - mem->field_names[fieldI].begin())-1;
    Serial.println("optionI = " + String(optionI));
    if (optionI < mem->fields[fieldI].size()) { // if option exists, remove option
        mem->field_names[fieldI].erase(mem->field_names[fieldI].begin() + optionI+1);
        mem->fields[fieldI].erase(mem->fields[fieldI].begin() + optionI);
        mem->rules[fieldI].erase(mem->rules[fieldI].begin() + optionI);
        Serial.println("removed option");
        writeMem("/mem", mem);
        editField(server, mem, "option was removed.");
    } else {
        editField(server, mem, "option not found");
    }
}

void editFieldEditRule(ESP8266WebServer& server, Mem* mem) {
    int fieldI = findField(server.arg("field"), mem);
    unsigned int optionI = (find(mem->field_names[fieldI].begin(), mem->field_names[fieldI].end(), server.arg("option")) - mem->field_names[fieldI].begin())-1;
    Serial.println("optionI = " + String(optionI));
    if (optionI < mem->fields[fieldI].size()) {
        // get disabled fields
        vector<String> disabled_fields = vector<String>();
        String ruleStr = server.arg("edit_rule");
        int pos = 0;
        int start = 0;
        String token;
        while ((pos = ruleStr.indexOf(',')) != -1) {    // while have extra comma
            token = ruleStr.substring(start, pos);
            if (findField(token, mem) != -1) {    // if field exists
                disabled_fields.push_back(token);
                Serial.println(token);
            }
            start = pos + 1;
        }
        if (findField(ruleStr.substring(start), mem) != -1) {   // if field exists
            disabled_fields.push_back(ruleStr.substring(start));
            Serial.println(ruleStr.substring(start));
        }

        // save rules
        mem->rules[fieldI][optionI] = disabled_fields;
        writeMem("/mem", mem);
        editField(server, mem, "updated disabled fields");

    } else {
        editField(server, mem, "invalid option");
    }
}

