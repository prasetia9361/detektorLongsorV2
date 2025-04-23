#pragma once

#include <Arduino.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <ArduinoJson.h>
#include "storage.h"

const int maxEspNowPacketSize = 127;

class commEspNow {
private:
    typedef struct message {
        char data[8] = "";
        int soil;
        int angle;
    } message;
    
    message messageData;

    storage* memoryStorage;
    uint8_t wifiChannel;
    String level = "";
    int bufferSize;
    int index;
    int headerSize;
    int lastData;
    int buttonValue = 0;
    bool stateBinding = false;
    char dataFromReceiver[12] = "";

public:
    commEspNow(storage* memoryStorage, uint8_t wifiChannel);
    bool begin();
    void addPeer();
    void sendData(int soil, int angle, const char* level);
    
    // Fungsi binding
    void statusBinding();
    bool getBinding() { return stateBinding; }

    String getLevel(){return level;}
    int getSoil(){return messageData.soil;}
    int getAngle(){return messageData.angle;}
    
    // Getter
    const char* getReceivedMessage() { return dataFromReceiver; }
    
    // Friend fungsi untuk callback
    friend void receiverCallback(const uint8_t* macAddr, const uint8_t* data, int dataLen);
};

// Callback function declarations
void receiverCallback(const uint8_t* macAddr, const uint8_t* data, int dataLen);