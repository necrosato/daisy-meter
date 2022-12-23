//
// daisy_meter.ino
// Naookie Sato
//
// Created by Naookie Sato on 12/21/2022
// Copyright © 2022 Naookie Sato. All rights reserved.
//
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include "secrets.h"

int samplethresh = 50;
int movementthresh = 15;
int timeout = 30 * 1000;

int outpins[] = { D1, D2, D3, D4, D5 };
#define npins 5
int val[npins] = {0};
int diff[npins] = {0};
int last[npins] = {0};
int samples[1024] = {0};
int sample = -1;
int dtime = 50;

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);
    for ( int i = 0; i < npins; i++ ) { pinMode( outpins[i], OUTPUT ); }
    Serial.begin(115200);
    WiFi.begin(ssid, pass);
    while (WiFi.status() != WL_CONNECTED) { delay(1000); Serial.println("Connecting..."); }
    digitalWrite(LED_BUILTIN, HIGH);
}

void trigger(String msg) {
    HTTPClient http;
    http.begin(endpoint);
    http.addHeader("Authorization", String("Bearer ") + llat);
    http.addHeader("Content-Type", "application/json");
    String data = "{\"state\": \"triggered\", \"attributes\": {\"msg\": \"" + msg + "\"}}";
    int response = http.POST(data);
    String payload = http.getString();
    Serial.println(String("Home Assistant Response Code: ") + String(response) + " payload: " + payload + " data: " + data);
}

void resetSamples() {
    memset(samples, 0, sizeof(samples)/sizeof(samples[0]));
    sample = -1;
    Serial.println("Samples reset");
}

String logValues() {
    String msg = "";
    for ( int i = 0; i < npins; i++ ) {
        msg += "Samples: " + String(sample+1) + " ";
        msg += "Location: " + String(i) + " Diff: " + String(diff[i]) + " Val: "  + String(val[i]) + "\n";
    }
    Serial.println(msg);
    return msg;
}

String checkMovements() {
    String msg = "";
    for ( int i = 0; i < npins; i++ ) {
        digitalWrite(outpins[i], HIGH);
        val[i] = analogRead(A0);
        digitalWrite(outpins[i], LOW);
        diff[i] = last[i] - val[i];
        if (diff[i] >= movementthresh || diff[i] * -1 >= movementthresh) {
            msg += "Movement at location " + String(i) + " Diff: " + String(diff[i]) + " >= " + String(movementthresh) + "\n";
        }
        last[i] = val[i];
    }
    return msg;
}

void loop() {
    int time = millis();
    if ( sample >= 0 && time - samples[sample] >= timeout ) resetSamples;
    auto msg = checkMovements();
    if (msg != "") {
        logValues();
        samples[++sample] = time;
    }
    if (sample+1 >= samplethresh) {
        logValues();
        Serial.println(msg);
        digitalWrite(LED_BUILTIN, LOW);
        trigger("triggered at time " + String(time));
        digitalWrite(LED_BUILTIN, HIGH);
        resetSamples();
    }
    delay(dtime);
}
