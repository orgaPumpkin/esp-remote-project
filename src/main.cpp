#include <Arduino.h>
#include "web.h"
#include "mfs.h"
#include "utils.h"

using namespace std;

void handleLed();
void handleRoot();
void handleLogin();

void handleRemote();
void handleEdit();
void handleSetup();

void handleEditField();
void handleSchedules();
void handleNotFound();

constexpr int led_pin = LED_BUILTIN;
constexpr int ir_pin = D2;
constexpr int sensor_pin = D1;

String g_led_state = "OFF";

// Set web server port number to 80
ESP8266WebServer server(80);

Mem* g_mem;
Schedules* g_schedules;
String g_profile = "";
String g_wifi_ssid;
String g_wifi_pass;
String g_username;
String g_password;

vector<String> g_profiles;

void setup() {
    Serial.begin(9600);
    Serial.println("\nstart");

    pinMode(led_pin, OUTPUT);
    digitalWrite(led_pin, LOW);
    pinMode(ir_pin, OUTPUT);
    pinMode(sensor_pin, INPUT);

    // files setup
    mountFS();
    delay(2000);

    File f = LittleFS.open("/credentials.txt", "r");
    g_wifi_ssid = "";
    readString(f, g_wifi_ssid, '\n');
    g_wifi_pass = "";
    readString(f, g_wifi_pass, '\n');
    g_username = "";
    readString(f, g_username, '\n');
    g_password = "";
    readString(f, g_password, '\n');
    f.close();

    g_profiles = vector<String>();
    f = LittleFS.open("/profiles.txt", "r");
    String line;
    while (f.available()) {
        readString(f, line, '\n');
        g_profiles.push_back(line);
    }
    f.close();

    getProfile(g_profiles[0], g_profile, g_profiles, g_mem, g_schedules);

    // web server setup
    startWIFI(g_wifi_ssid, g_wifi_pass);
    server.collectHeaders("Cookie");
    server.on("/", handleRoot);
    server.on("/led", handleLed);
    server.on("/login", handleLogin);
    server.on("/remote", handleRemote);
    server.on("/edit", handleEdit);
    server.on("/setup", handleSetup);
    server.on("/edit_field", handleEditField);
    server.on("/schedules", handleSchedules);
    server.serveStatic("/stylesheet.css", LittleFS, "/stylesheet.css");
    server.onNotFound(handleNotFound);
    server.begin();
    digitalWrite(led_pin, HIGH);
}


// handlers

void handleLed() {
    if (checkAuth(server)) { // user is authenticated
        ledPage(server, g_led_state, led_pin);

    } else {
        server.sendHeader("Location", "/login");
        server.send(302);
    }
}

void handleRoot() {
    if (checkAuth(server)) {
        if (server.method() == HTTP_POST) {
            if (server.hasArg("delete")) {
                rootRemove(server, g_profiles);

            } else if (server.hasArg("add")) {
                rootAdd(server, g_profiles);

            }
        } else {
            rootShow(server, g_profiles);
        }
    } else {
        server.sendHeader("Location", "/login");
        server.send(302);
    }
}

void handleLogin() {
    if (checkAuth(server)) { // user is authenticated
        if (server.hasArg("disconnect")) {
            loginDisconnect(server);
        } else {
            loginConnected(server);
        }

    } else if (server.method() == HTTP_POST && server.hasArg("username") && server.hasArg("password")) { // login post
        loginLogin(server, g_username, g_password);

    } else {    // normal page get
        String html = "";
        readFile("/login.html", &html);
        html.replace("%s", "");
        server.send(200, "text/html", html);
    }
}


void handleRemote() {
    if (checkAuth(server)) {    // user is authenticated
        if (getProfile(server.arg("profile"), g_profile, g_profiles, g_mem, g_schedules)) {
            if (server.method() == HTTP_POST) {

                if (server.hasArg("toggle")) {
                    remoteToggle(server, g_mem, ir_pin);

                } else {
                    remoteSendData(server, g_mem, ir_pin);
                }

            } else {
                remoteShow(server, g_mem, "");
            }
        } else {
            server.sendHeader("Location", "/");
            server.send(302);
        }
    } else {
        server.sendHeader("Location", "/login");
        server.send(302);
    }
}

void handleEdit() {
    if (checkAuth(server)) {
        // user is authenticated
        if (getProfile(server.arg("profile"), g_profile, g_profiles, g_mem, g_schedules)) {
            if (server.method() == HTTP_POST) {
                if (server.hasArg("add_toggle")) {
                    editAddToggle(server, g_mem, sensor_pin, led_pin);
                    writeMem(g_profile, g_mem);

                } else if (server.hasArg("remove_toggle")) {
                    editRemoveToggle(server, g_mem);
                    writeMem(g_profile, g_mem);

                } else if (server.hasArg("add_field")) {
                    editAddField(server, g_mem);
                    writeMem(g_profile, g_mem);

                } else if (server.hasArg("remove_field")) {
                    editRemoveField(server, g_mem);
                    writeMem(g_profile, g_mem);
                }

                if (server.hasArg("record_base")) {
                    editRecordBase(server, g_mem, sensor_pin, led_pin);
                    writeMem(g_profile, g_mem);

                } else if (server.hasArg("reset")) {
                    editReset(server, g_mem, g_schedules);
                    writeMem(g_profile, g_mem);
                    writeSchedule(g_schedules, g_profile);

                } else if (server.hasArg("base")) {
                    editSendBase(server, g_mem, ir_pin);
                }

            } else {
                editShow(server, g_mem, "");
            }

        } else {
            server.sendHeader("Location", "/");
            server.send(302);
        }
    } else {
        server.sendHeader("Location", "/login");
        server.send(302);
    }
}


void handleSetup() {
    if (checkAuth(server)) {    // user is authenticated
        if (server.method() == HTTP_POST) {
            if (server.hasArg("wifi_ssid") && server.hasArg("wifi_pass")) {
                setupWifi(server, g_wifi_ssid, g_wifi_pass);
                File f = LittleFS.open("credentials.txt", "w");
                if (f) {
                    f.print(g_wifi_ssid+'\n');
                    f.print(g_wifi_pass+'\n');
                    f.print(g_username+'\n');
                    f.print(g_password+'\n');
                    f.close();
                }

            } else if (server.hasArg("username") && server.hasArg("password")) {
                setupUser(server, g_username, g_password);
                File f = LittleFS.open("credentials.txt", "w");
                if (f) {
                    f.print(g_wifi_ssid+'\n');
                    f.print(g_wifi_pass+'\n');
                    f.print(g_username+'\n');
                    f.print(g_password+'\n');
                    f.close();
                }
            }
        }
        setupShow(server, "");
    } else {
        server.sendHeader("Location", "/login");
        server.send(302);
    }
}

void handleEditField() {
    if (checkAuth(server)) {
        if (getProfile(server.arg("profile"), g_profile, g_profiles, g_mem, g_schedules)) {
            if (server.hasArg("field") && findField(server.arg("field"), g_mem) != -1) {
                if (server.method() == HTTP_POST) {
                    if (server.hasArg("add_option")) {
                        editFieldAddOption(server, g_mem, sensor_pin, led_pin);

                    } else if (server.hasArg("remove_option")) {
                        editFieldRemoveOption(server, g_mem);

                    } else if (server.hasArg("edit_rule") && server.hasArg("option")) {
                        editFieldEditRule(server, g_mem);
                    }
                    writeMem(g_profile, g_mem);

                } else {
                    editField(server, g_mem, "");
                }
            } else {
                server.send(401, "text/plain", "invalid field");
            }
        } else {
            server.sendHeader("Location", "/");
            server.send(302);
        }
    } else {
        server.sendHeader("Location", "/login");
        server.send(302);
    }
}

void handleSchedules() {
    if (checkAuth(server)) {
        if (server.method() == HTTP_POST) {
            if (server.hasArg("edit_schedule")) {

            }
        }

    } else {
        server.sendHeader("Location", "/login");
        server.send(302);
    }
}

void handleNotFound() {
    server.send(404, "text/plain", "page not found");
}


// main loop

void loop() {
    if (WiFi.status() == WL_CONNECTED || WiFi.softAPgetStationNum() > 0) {
        server.handleClient();
    } else {
        Serial.println("WiFi not connected");
        delay(500);
    }
}
