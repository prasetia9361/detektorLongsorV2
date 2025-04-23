#pragma once 
#include <Arduino.h>
#include <driver/gpio.h>
#include <MQTT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <esp_task_wdt.h>

#include "commEspNow.h"
#include "storage.h"
#include "button/button.h"
#include "clientServer/clientServer.h"
#include "wifiAP/wifiAP.h"

#define BINDING_BUTTON GPIO_NUM_0

class receiver
{
private:
    commEspNow *comm;
    storage *memory;
    button *mButton;
    clientServer *server;
    wifiAP *wifi;

    String ssid = "";
    String pass = "";

    // MQTT Configuration
    const char* MQTT_BROKER = "privateghofinda.cloud.shiftr.io";
    const char* MQTT_USER = "privateghofinda";
    const char* MQTT_PASS = "Jqi8J4DAALtisiMk";
    String MQTT_TOPIC_PREFIX = "3013042/longsor";
    SemaphoreHandle_t mqttMutex;

    void connectToMQTT(); 
    void checkWiFiStatus();
public:
    receiver();
    void init();
    void processBinding();
    void beginEspNowandLcd();
    void kredensialWifi();
    void publishData();
    void mqttLoop();
    void printLcd();
    void wdtReset();
    friend void mqttCallback(String &topic, String &payload);
};
void mqttCallback(String &topic, String &payload);
