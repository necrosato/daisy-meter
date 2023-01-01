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

#define SAMPLE_RATE 50
#define SAMPLE_DETECT_THRESHOLD 15
#define SAMPLES_PER_TRIGGER 50
#define SAMPLES_RESET_TIMEOUT 30*1000

// dont use d4 or else on board led will not work
int outpins[] = {D0, D5, D6, D7, D8 };
#define NPINS 5
#define LED_ON digitalWrite(LED_BUILTIN, LOW)
#define LED_OFF digitalWrite(LED_BUILTIN, HIGH)
int val[NPINS] = {0};
int diff[NPINS] = {0};
int last[NPINS] = {0};
int samples[SAMPLES_PER_TRIGGER] = {0};
int sample = -1;

void setup() {
    Serial.begin(115200);
    pinMode(LED_BUILTIN, OUTPUT);
    for ( int i = 0; i < NPINS; i++ ) { pinMode( outpins[i], OUTPUT ); }
    LED_ON;
    WiFi.begin(ssid, pass);
    while (WiFi.status() != WL_CONNECTED) { delay(1000); Serial.println("Connecting..."); }
    LED_OFF;
}

void sendState(String state, String msg) {
    HTTPClient http;
    http.begin(endpoint);
    http.addHeader("Authorization", String("Bearer ") + llat);
    http.addHeader("Content-Type", "application/json");
    String data = "{\"state\": \"" + state + "\", \"attributes\": {\"msg\": \"" + msg + "\"}}";
    int response = http.POST(data);
    String payload = http.getString();
    Serial.println(String("Home Assistant Response Code: ") + String(response) + "\npayload:\n\t" + payload + "\ndata:\n\t" + data);
}

void resetSamples() {
    memset(samples, 0, sizeof(samples)/sizeof(samples[0]));
    sample = -1;
    Serial.println("Samples reset");
}

void logValues(String m="") {
    String msg = "Buffered Time Samples: " + String(sample+1) + "\n\t";
    for ( int i = 0; i < NPINS; i++ ) {
        msg += "Location: " + String(i) + " Val: "  + String(val[i]) + " Diff: " + String(diff[i]) + "\n\t";
    }
    Serial.println(m + msg);
}

bool readValues() {
    bool detected = false;
    for ( int i = 0; i < NPINS; i++ ) {
        digitalWrite(outpins[i], HIGH);
        diff[i] = val[i] - analogRead(A0);
        digitalWrite(outpins[i], LOW);
        val[i] = val[i] - diff[i];
        if (diff[i] >= SAMPLE_DETECT_THRESHOLD || diff[i] * -1 >= SAMPLE_DETECT_THRESHOLD) {
            //Serial.println("Sample passed threshold at location " + String(i) + " Diff: " + String(diff[i]) + " >= " + String(SAMPLE_DETECT_THRESHOLD) + "\n");
            detected = true;
        }
    }
    return detected;
}

void loop() {
    int time = millis();
    if ( sample >= 0 && time - samples[sample] >= SAMPLES_RESET_TIMEOUT ) resetSamples();
    if (readValues()) {
        samples[++sample] = time;
    }
    if (sample+1 >= SAMPLES_PER_TRIGGER) {
        logValues();
        LED_ON;
        sendState("triggered", "running time: " + String(time));
        delay(100);
        LED_OFF;
        resetSamples();
    }
    delay(SAMPLE_RATE);
}
