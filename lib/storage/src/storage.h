#ifndef STORAGE_H
#define STORAGE_H

// #include <Arduino.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include "FS.h"
#include "SD.h"

class storage{
private:
    typedef struct config {
        uint8_t macAddress[6] = {0,0,0,0,0,0};
        uint8_t macAddress1[6] = {0,0,0,0,0,0};
    } config;
    config configData;

    typedef struct wifi
    {
        String ssid;
        String pass;
    }wifi;
    wifi setWifi;
    

    String receivedData;
    uint8_t macAddr[6];

public:
    storage();
    ~storage();

    uint8_t *getMac(){return configData.macAddress;}
    uint8_t *getMac1(){return configData.macAddress1;}

    void readWifi();
    String getSsid() { return setWifi.ssid; }
    String getPass() {return setWifi.pass;}

    void init();
    void writeMacAddress(const uint8_t *mac, int count);
    void writeWifi(const String& ssid, const String& pass);
    void deleteAddress();
};
#endif