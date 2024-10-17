#include <Arduino.h>
#include "web.h"
#include "mfs.h"
#include "utils.h"

using namespace std;

void handleRoot();
void handleLed();
void handleLogin();
void handleBase();
void handleRemote();
void handleEditField();
void handleNotFound();

constexpr int led_pin = LED_BUILTIN;
constexpr int ir_pin = D2;
constexpr int sensor_pin = D1;

String g_led_state = "OFF";

// Set web server port number to 80
ESP8266WebServer server(80);

Mem* g_mem;
String g_wifi_ssid;
String g_wifi_pass;
String g_username;
String g_password;
vector<String> profiles = vector<String>();
String g_memPath;

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
    Serial.print(g_wifi_ssid);
    Serial.print(",");
    Serial.print(g_wifi_pass);
    Serial.println(".");

    f = LittleFS.open("/profiles.txt", "r");
    while (f.available()) {
        profiles.push_back(f.readStringUntil('\n'));
    }
    f.close();
    g_memPath = profiles[0] + ".mem";

    if (!LittleFS.exists(g_memPath)) {
        g_mem = new Mem;
        g_mem->error_us = 300;
        g_mem->base_message = vector<int>();
        g_mem->low_ranges = vector<int>();
        g_mem->high_ranges = vector<int>();
        g_mem->toggle_names = vector<String>();
        g_mem->field_names = vector<vector<String>>();
        g_mem->toggles = vector<vector<int>>();
        g_mem->fields = vector<vector<vector<int>>>();
        g_mem->rules = vector<vector<vector<String>>>();
        Serial.println("creating mem");
        writeMem(g_memPath, g_mem);
    }
    else {
        Serial.println("mem already exists");
        // dumpFile(profile+".mem");
        g_mem = readMem(g_memPath);
    }

    // web server setup
    startWIFI(g_wifi_ssid, g_wifi_pass);
    server.collectHeaders("Cookie");
    server.on("/", handleRoot);
    server.on("/led", handleLed);
    server.on("/login", handleLogin);
    server.on("/base", handleBase);
    server.on("/remote", handleRemote);
    server.on("/edit_field", handleEditField);
    server.onNotFound(handleNotFound);
    server.begin();
    digitalWrite(led_pin, HIGH);
}


// handlers

void handleRoot() {
    if (checkAuth(server)) {
        server.sendHeader("Location", "/remote");
        server.send(302);
    } else {
        server.sendHeader("Location", "/login");
        server.send(302);
    }
}

void handleLed() {
    if (checkAuth(server)) { // user is authenticated
        ledPage(server, g_led_state, led_pin);

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

void handleBase() {
    if (checkAuth(server)) {    // user is authenticated
        if (server.method() == HTTP_POST && server.hasArg("action")) {
            if (server.arg("action") == "record") {
                baseRecord(server, g_mem, sensor_pin, led_pin);

            } else if (server.arg("action") == "reset") {
                baseReset(server, g_mem);
            }
            writeMem(g_memPath, g_mem);

        } else {
            String html = "";
            readFile("/base.html", &html);
            html.replace("%s", "");
            server.send(200, "text/html", html);
        }
    } else {
        server.sendHeader("Location", "/login");
        server.send(302);
    }
}

void handleRemote() {
    if (checkAuth(server)) {    // user is authenticated

        if (server.method() == HTTP_POST) {
            if (server.hasArg("base")) {
                remoteBase(server, g_mem, ir_pin);

            } else if (server.hasArg("toggle")) {
                remoteToggle(server, g_mem, ir_pin);

            } else if (server.hasArg("add_toggle")) {
                remoteAddToggle(server, g_mem, sensor_pin, led_pin);
                writeMem(g_memPath, g_mem);

            } else if (server.hasArg("remove_toggle")) {
                remoteRemoveToggle(server, g_mem);
                writeMem(g_memPath, g_mem);

            } else if (server.hasArg("add_field")) {
                remoteAddField(server, g_mem);
                writeMem(g_memPath, g_mem);

            } else if (server.hasArg("remove_field")) {
                remoteRemoveField(server, g_mem);
                writeMem(g_memPath, g_mem);

            } else if (server.hasArg("wifi_ssid") && server.hasArg("wifi_pass")) {
                remoteWifi(server, g_mem, g_wifi_ssid, g_wifi_pass);
                File f = LittleFS.open("credentials.txt", "w");
                if (f) {
                    f.print(g_wifi_ssid+'\n');
                    f.print(g_wifi_pass+'\n');
                    f.print(g_username+'\n');
                    f.print(g_password+'\n');
                    f.close();
                }

            } else if (server.hasArg("username") && server.hasArg("password")) {
                remoteEditUser(server, g_mem, g_username, g_password);
                File f = LittleFS.open("credentials.txt", "w");
                if (f) {
                    f.print(g_wifi_ssid+'\n');
                    f.print(g_wifi_pass+'\n');
                    f.print(g_username+'\n');
                    f.print(g_password+'\n');
                    f.close();
                }

            } else {
                remoteSendData(server, g_mem, ir_pin);
            }

        } else {
            remote(server, g_mem, "");
        }
    } else {
        server.sendHeader("Location", "/login");
        server.send(302);
    }
}

void handleEditField() {
    if (checkAuth(server)) {
        // user is authenticated
        if (server.hasArg("field") && findField(server.arg("field"), g_mem) != -1) {
            if (server.method() == HTTP_POST) {
                if (server.hasArg("add_option")) {
                    editFieldAddOption(server, g_mem, sensor_pin, led_pin);

                } else if (server.hasArg("remove_option")) {
                    editFieldRemoveOption(server, g_mem);

                } else if (server.hasArg("edit_rule") && server.hasArg("option")) {
                    editFieldEditRule(server, g_mem);
                }
                writeMem(g_memPath, g_mem);

            } else {
                editField(server, g_mem, "");
            }
        } else {
            server.send(401, "text/plain", "invalid field");
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
