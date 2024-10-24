#include "web.h"

#include <NTPClient.h>

#include "mfs.h"

void startWIFI(String& ssid, String& pass) {
    if (ssid != "") {
        // connect to WiFi
        Serial.println("Connecting to " + ssid);
        WiFi.begin(ssid, pass);
        unsigned long st = millis();
        while (WiFi.status() != WL_CONNECTED && millis()-st < 1000*30) {  // Wait for the Wi-Fi connection completion
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

void loginLogin(ESP8266WebServer& server, String username, String password) {
    if (server.arg("username") == username && server.arg("password") == password) { // right login
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

void rootShow(ESP8266WebServer& server, vector<String>& profiles) {
    String html = "";
    readFile("/index.html", &html);

    String profilesStr = "";
    for (const String& profile : profiles) {
        profilesStr += "\""+profile+"\",";
    }
    if (profilesStr.length() >= 1) {
        profilesStr.remove(profilesStr.length() - 1);
    }
    html.replace("{profiles}", profilesStr);

    server.send(200, "text/html", html);
}


void rootRemove(ESP8266WebServer& server, vector<String>& profiles) {
    if (profiles.size() > 1) {
        unsigned int found = findElement(server.arg("delete"), profiles);
        if (found < profiles.size()) {
            profiles.erase(profiles.begin() + found);
            LittleFS.remove(server.arg("delete")+".mem");

            File f = LittleFS.open("/profiles.txt", "w");
            for (String line : profiles) {
                f.print(line + '\n');
            }
            f.close();

            rootShow(server, profiles);
            return;
        }
    }
    server.send(403, "text/html", "failed");
}

void rootAdd(ESP8266WebServer& server, vector<String>& profiles) {
    if (count(profiles.begin(), profiles.end(), server.arg("add")) == 0) {
        profiles.push_back(server.arg("add"));

        File f = LittleFS.open("/profiles.txt", "w");
        for (String line : profiles) {
            f.print(line + '\n');
        }

        server.sendHeader("HX-Refresh", "true");
        rootShow(server, profiles);
    } else {
        server.send(403, "text/html", "failed");
    }
}


// remote
void remoteShow(ESP8266WebServer& server, Mem* mem, const String& message) {
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
    html.replace("{fields}", formatStr);

    formatStr = "";
    for (String& toggle_name : mem->toggle_names) {
        formatStr += toggle_name + ",";
    }
    if (formatStr.length() >= 1) {
        formatStr.remove(formatStr.length() - 1);
    }
    html.replace("{toggles}", formatStr);

    formatStr = "";
    for (int option : mem->last_options) {
        formatStr += String(option) + ",";
    }
    if (formatStr.length() >= 1) {
        formatStr.remove(formatStr.length() - 1);
    }
    html.replace("{last}", formatStr);

    html.trim();
    server.send(200, "text/html", html);
}




void remoteToggle(ESP8266WebServer& server, Mem* mem, int ir) {
    unsigned int toggleI = findElement(server.arg("toggle"), mem->toggle_names);

    if (toggleI < mem->toggle_names.size()) {   // toggle exists
        sendMessage(mem->toggles[toggleI], ir, mem);
        remoteShow(server, mem, "sent toggle.");
    } else {
        remoteShow(server, mem, "invalid toggle");
    }
}

void remoteSendData(ESP8266WebServer& server, Mem* mem, int ir) {
    vector<fieldValue> fields = getFieldsServer(server, mem);
    vector<int> message = buildDataMessage(fields, mem);

    sendMessage(message, ir, mem);
    remoteShow(server, mem, "sent data message.");
}

// edit
void editShow(ESP8266WebServer& server, Mem* mem, const String& message) {
    String html = "";
    readFile("/edit.html", &html);
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
    html.replace("{fields}", formatStr);

    formatStr = "";
    for (String& toggle_name : mem->toggle_names) {
        formatStr += toggle_name + ",";
    }
    if (formatStr.length() >= 1) {
        formatStr.remove(formatStr.length() - 1);
    }
    html.replace("{toggles}", formatStr);


    server.send(200, "text/html", html);
}


void editAddToggle(ESP8266WebServer& server, Mem* mem, int sensor, int led) {
    // get message
    vector<int> raw_message = vector<int>();
    if (getMessage(raw_message, sensor, led)) {  // if got a message
        unsigned int  toggleI = distance(mem->toggle_names.begin(), find(mem->toggle_names.begin(), mem->toggle_names.end(), server.arg("add_toggle")));
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

        editShow(server, mem, "constant was recorded.");
    } else {
        editShow(server, mem, "no message was detected. try again.");
    }
}

void editRemoveToggle(ESP8266WebServer& server, Mem* mem) {
    unsigned int  toggleI = distance(mem->toggle_names.begin(), find(mem->toggle_names.begin(), mem->toggle_names.end(), server.arg("remove_toggle")));
    if (toggleI < mem->toggle_names.size()) { // if toggle exists
        // remove toggle
        mem->toggle_names.erase(mem->toggle_names.begin() + toggleI);
        mem->toggles.erase(mem->toggles.begin() + toggleI);
        Serial.println("removed toggle");
        editShow(server, mem, "constant was removed.");
    } else {
        editShow(server, mem, "constant not found");
    }
}

void editAddField(ESP8266WebServer& server, Mem* mem) {
    int fieldI = findField(server.arg("add_field"), mem);
    // if field doesn't already exist, create
    if (fieldI == -1) {
        mem->field_names.emplace_back();
        mem->field_names[mem->field_names.size()-1].push_back(server.arg("add_field"));
        mem->fields.emplace_back();
        mem->last_options.emplace_back();
        mem->rules.emplace_back();
        Serial.println("added field");
    }
    server.sendHeader("Location", "/edit_field?profile=" + server.arg("profile") + "&field=" + server.arg("add_field"));
    server.send(302);
}

void editRemoveField(ESP8266WebServer& server, Mem* mem) {

    int fieldI = findField(server.arg("remove_field"), mem);
    if (fieldI != -1) { // if field exists
        // remove toggle
        mem->field_names.erase(mem->field_names.begin() + fieldI);
        mem->fields.erase(mem->fields.begin() + fieldI);
        mem->last_options.erase(mem->last_options.begin() + fieldI);
        mem->rules.erase(mem->rules.begin() + fieldI);
        Serial.println("removed field");
        editShow(server, mem, "field was removed.");
    } else {
        editShow(server, mem, "field not found");
    }
}

void editSendBase(ESP8266WebServer& server, Mem* mem, int ir) {
    sendMessage(mem->base_message, ir, mem);
    editShow(server, mem, "sent base data message.");
}

void editRecordBase(ESP8266WebServer& server, Mem* mem, int sensor, int led) {
    vector<int> raw_message = vector<int>();
    if (getMessage(raw_message, sensor, led)) {

        mem->error_us = 0;
        for (int pulse : raw_message) {
            mem->error_us += pulse;
        }
        mem->error_us = mem->error_us / raw_message.size()/100 * 40;
        Serial.println("error us: " + String(mem->error_us));

        vector<int> message = vector<int>();
        processMessage(raw_message, message, mem);
        mem->base_message = message;

        editShow(server, mem, "Length: "+ String(raw_message.size()));
    } else {
        editShow(server, mem, "no message was detected. try again.");
    }
}

void editReset(ESP8266WebServer& server, Mem* mem, Schedules* schedules) {
    mem->low_ranges.clear();
    mem->high_ranges.clear();
    mem->base_message.clear();
    mem->field_names.clear();
    mem->fields.clear();
    mem->toggle_names.clear();
    mem->toggles.clear();
    mem->error_us = 300;
    mem->rules.clear();

    schedules->data_schedules.clear();
    schedules->toggle_schedules.clear();

    editShow(server, mem, "Remote reset");
}

void setupShow(ESP8266WebServer& server, const String& message) {
    String html = "";
    readFile("/setup.html", &html);
    html.replace("%s", message);
    server.send(200, "text/html", html);
}

void setupWifi(ESP8266WebServer& server, String& ssid, String& pass) {
    ssid = server.arg("wifi_ssid");
    pass = server.arg("wifi_pass");
    setupShow(server, "WiFi details updated, restart esp to apply changes.");
}

void setupUser(ESP8266WebServer& server, String& username, String& password) {
    username = server.arg("username");
    password = server.arg("password");
    setupShow(server, "login details updated.");
}

void setupTimeZone(ESP8266WebServer& server, NTPClient& ntp) {
    int timezone = server.arg("timezone").toInt();
    File f = LittleFS.open("/timezone", "w");
    writeBytes(f, timezone);
    f.close();
    ntp.setTimeOffset(3600*timezone);
    setupShow(server, "Time zone set");
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
        String disable_fields = "";
        vector<bool> effected = findEffected(fieldI, mem);
        int effected_count = count(effected.begin(), effected.end(), true);
        for (unsigned int i = 0; i < mem->fields.size(); i++) {
            if (i != static_cast<unsigned>(fieldI)) {
                vector<bool> field_effected = findEffected(i, mem);
                int field_effected_count = count(field_effected.begin(), field_effected.end(), true);
                if (field_effected_count < effected_count) {
                    for (unsigned int j = 0; j < effected.size(); j++) {
                        if (effected[j] && field_effected[j]) {
                            disable_fields += mem->field_names[i][0] + ",";
                            break;
                        }
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
            unsigned int optionI = findElement(server.arg("add_option"), mem->field_names[fieldI])-1;
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
    unsigned int optionI = findElement(server.arg("remove_option"), mem->field_names[fieldI])-1;
    if (optionI < mem->fields[fieldI].size()) { // if option exists, remove option
        mem->field_names[fieldI].erase(mem->field_names[fieldI].begin() + optionI+1);
        mem->fields[fieldI].erase(mem->fields[fieldI].begin() + optionI);
        mem->rules[fieldI].erase(mem->rules[fieldI].begin() + optionI);
        Serial.println("removed option");
        editField(server, mem, "option was removed.");
    } else {
        editField(server, mem, "option not found");
    }
}

void editFieldEditRule(ESP8266WebServer& server, Mem* mem) {
    int fieldI = findField(server.arg("field"), mem);
    unsigned int optionI = findElement(server.arg("option"), mem->field_names[fieldI])-1;
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
                Serial.println("disabled: " + token);
            }
            start = pos + 1;
        }
        if (findField(ruleStr.substring(start), mem) != -1) {   // if field exists
            disabled_fields.push_back(ruleStr.substring(start));
            Serial.println("disabled: " + ruleStr.substring(start));
        }

        // save rules
        mem->rules[fieldI][optionI] = disabled_fields;
        editField(server, mem, "updated disabled fields");

    } else {
        editField(server, mem, "invalid option");
    }
}

void schedulesShow(ESP8266WebServer& server, Schedules* schedules, vector<String>& profiles, const String& message) {
    String html = "";
    readFile("/schedules.html", &html);
    html.replace("%s", message);

    String allProfilesStr = "";
    for (const auto & profile : profiles) {
        allProfilesStr += profile + ",";
    }
    if (allProfilesStr.length() > 0) {
        allProfilesStr.remove(allProfilesStr.length() - 1);
    }
    html.replace("{all-profiles}", allProfilesStr);


    String schedulesStr ="";
    String profilesStr ="";
    String daysStr ="";
    String timesStr ="";

    for (DataSchedule& schedule : schedules->data_schedules) {
        schedulesStr += schedule.name +",";
        profilesStr += schedule.profile +",";

        for (bool day : schedule.time.days) { daysStr += String(day); }
        daysStr += ",";
        char time[6] = "";
        sprintf(time, "%02d:%02d", schedule.time.hour, schedule.time.minute);
        timesStr += String(time) + ",";
    }
    for (ToggleSchedule& schedule : schedules->toggle_schedules) {
        schedulesStr += schedule.name +",";
        profilesStr += schedule.profile +",";

        for (bool day : schedule.time.days) { daysStr += String(day); }
        daysStr += ",";
        char time[6] = "";
        sprintf(time, "%02d:%02d", schedule.time.hour, schedule.time.minute);
        timesStr += String(time) + ",";
    }
    if (schedulesStr.length() > 0) {
        schedulesStr.remove(schedulesStr.length() - 1);
        profilesStr.remove(profilesStr.length() - 1);
        daysStr.remove(daysStr.length() - 1);
        timesStr.remove(timesStr.length() - 1);
    }


    html.replace("{names}", schedulesStr);
    html.replace("{profiles}", profilesStr);
    html.replace("{days}", daysStr);
    html.replace("{times}", timesStr);

    server.send(200, "text/html", html);
}


void schedulesAdd(ESP8266WebServer& server, vector<String>& profiles, Schedules* schedules) {
    if (findElement(server.arg("profile"), profiles) < profiles.size()) {
        for (DataSchedule& schedule : schedules->data_schedules) {
            if (schedule.name == server.arg("add")) {
                schedulesShow(server, schedules, profiles, "name already exists");
                return;
            }
        }
        for (ToggleSchedule& schedule : schedules->toggle_schedules) {
            if (schedule.name == server.arg("add")) {
                schedulesShow(server, schedules, profiles, "name already exists");
                return;
            }
        }

        // valid data
        Mem* mem;
        loadMem(mem, server.arg("profile"));

        if (server.arg("toggle") == "1") {
            ToggleSchedule new_schedule;
            new_schedule.name = server.arg("add");
            new_schedule.profile = server.arg("profile");
            for (bool & day : new_schedule.time.days){
                day = false;
            }
            new_schedule.time.hour = 0;
            new_schedule.time.minute = 0;

            if (!mem->toggle_names.empty()) {
                new_schedule.toggle_name = mem->toggle_names[0];
            } else {
                new_schedule.toggle_name = "";
            }

            schedules->toggle_schedules.push_back(new_schedule);

        } else {
            DataSchedule new_schedule;
            new_schedule.name = server.arg("add");
            new_schedule.profile = server.arg("profile");
            for (bool & day : new_schedule.time.days){
                day = false;
            }
            new_schedule.time.hour = 0;
            new_schedule.time.minute = 0;

            new_schedule.field_names = vector<String>();
            new_schedule.option_names = vector<String>();
            for (vector<String>& field : mem->field_names) {
                new_schedule.field_names.push_back(field[0]);
                new_schedule.option_names.push_back(field[1]);
            }

            schedules->data_schedules.push_back(new_schedule);
        }

        writeSchedule(schedules);
        schedulesShow(server, schedules, profiles, "added schedule");

    } else {
        schedulesShow(server, schedules, profiles, "no profile found");
    }

}

void schedulesRemove(ESP8266WebServer& server, vector<String>& profiles, Schedules* schedules) {
    for (unsigned int i=0; i<schedules->data_schedules.size(); i++) {
        if (schedules->data_schedules[i].name == server.arg("remove")) {
            schedules->data_schedules.erase(schedules->data_schedules.begin()+i);

            writeSchedule(schedules);
            schedulesShow(server, schedules, profiles, "schedule removed");
        }
    }

    for (unsigned int i=0; i<schedules->toggle_schedules.size(); i++) {
        if (schedules->toggle_schedules[i].name == server.arg("remove")) {
            schedules->toggle_schedules.erase(schedules->toggle_schedules.begin()+i);

            writeSchedule(schedules);
            schedulesShow(server, schedules, profiles, "schedule removed");
        }
    }

    schedulesShow(server, schedules, profiles, "schedule not found");
}


void editScheduleShow(ESP8266WebServer& server, Schedules* schedules, const String& message) {
    String html = "";

    for (DataSchedule& schedule : schedules->data_schedules) {
        if (schedule.name == server.arg("schedule")) {
            readFile("/edit_schedule_data.html", &html);
            html.replace("%s", message);

            String daysStr ="";
            for (bool day : schedule.time.days) { daysStr += String(day); }
            html.replace("{days}", daysStr);

            char timeStr[6] = "";
            sprintf(timeStr, "%02d:%02d", schedule.time.hour, schedule.time.minute);
            html.replace("{time}", timeStr);

            // get fields and options
            String fieldsStr = "";
            String optionsStr = "";
            Mem* mem;
            loadMem(mem, schedule.profile);
            for (vector<String> option_names : mem->field_names) {
                for (String& option_name : option_names) {
                    fieldsStr += option_name + ",";
                }
                fieldsStr[fieldsStr.length()-1] = ';';
                unsigned int fieldI = findElement(option_names[0], schedule.field_names);
                if (fieldI < schedule.field_names.size()) {
                    optionsStr += schedule.option_names[fieldI] + ",";
                } else {
                    optionsStr += ",";
                }
            }
            if (fieldsStr.length() >= 1) {
                fieldsStr.remove(fieldsStr.length() - 1);
                optionsStr.remove(optionsStr.length() - 1);
            }
            html.replace("{fields}", fieldsStr);
            html.replace("{options}", optionsStr);

            server.send(200, "text/html", html);
            return;
        }
    }

    for (ToggleSchedule& schedule : schedules->toggle_schedules) {
        if (schedule.name == server.arg("schedule")) {
            readFile("/edit_schedule_toggle.html", &html);
            html.replace("%s", message);

            String daysStr ="";
            for (bool day : schedule.time.days) { daysStr += String(day); }
            html.replace("{days}", daysStr);

            char timeStr[6] = "";
            sprintf(timeStr, "%02d:%02d", schedule.time.hour, schedule.time.minute);
            html.replace("{time}", timeStr);

            String togglesStr = "";
            Mem* mem;
            loadMem(mem, schedule.profile);
            for (String& toggle : mem->toggle_names) {
                togglesStr += toggle + ",";
            }
            if (togglesStr.length() >= 1) {
                togglesStr.remove(togglesStr.length() - 1);
            }
            html.replace("{toggles}", togglesStr);

            html.replace("{last}", schedule.toggle_name);

            server.send(200, "text/html", html);
            return;
        }
    }

    server.send(200, "text/html", "schedule not found");
}

void editSchedulesEdit(ESP8266WebServer& server, Schedules* schedules) {
    for (DataSchedule& schedule : schedules->data_schedules) {
        if (schedule.name == server.arg("schedule")) {

            for (int i=0; i < 7; i++) {
                if (server.arg("days")[i] == '1') {
                    schedule.time.days[i] = true;
                } else {
                    schedule.time.days[i] = false;
                }
            }

            schedule.time.hour = server.arg("time").substring(0, 2).toInt();
            schedule.time.minute = server.arg("time").substring(3, 5).toInt();

            // get fields and options
            schedule.field_names = vector<String>();
            schedule.option_names = vector<String>();
            for (int argI = 0; argI < server.args(); argI++) {  // for arg in request
                const String& argName = server.argName(argI);
                if (argName != "time" && argName != "days" && argName != "schedule") {
                    // extract all fields data
                    schedule.field_names.push_back(argName);
                    schedule.option_names.push_back(server.arg(argName));
                }
            }

            writeSchedule(schedules);
            editScheduleShow(server, schedules, "schedule updated");
            return;
        }
    }

    for (ToggleSchedule& schedule : schedules->toggle_schedules) {
        if (schedule.name == server.arg("schedule")) {

            for (int i=0; i < 7; i++) {
                if (server.arg("days")[i] == '1') {
                    schedule.time.days[i] = true;
                } else {
                    schedule.time.days[i] = false;
                }
            }

            schedule.time.hour = server.arg("time").substring(0, 2).toInt();
            schedule.time.minute = server.arg("time").substring(3, 5).toInt();

            schedule.toggle_name = server.arg("toggle");

            writeSchedule(schedules);
            editScheduleShow(server, schedules, "schedule updated");
            return;
        }
    }

    server.send(200, "text/html", "schedule not found");
}
