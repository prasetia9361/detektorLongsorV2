#pragma once
#include <Arduino.h>
#include <driver/gpio.h>
#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <string.h>
#include <esp_task_wdt.h>

#include "commEspNow.h"
#include "storage.h"
#include "button/button.h"

// Pin Definitions
#define SOIL_PIN        36
#define LED_RED         5
#define LED_YELLOW      16
#define LED_GREEN       17
#define SIRENE_PIN      18
#define I2C_SDA         21
#define I2C_SCL         22
#define BINDING_BUTTON GPIO_NUM_0


class transmitter
{
private:
    commEspNow *comEspnow;
    storage *memory;
    button *mButton;

    // Shared Variables with Mutex Protection
    volatile int soilMoisture;
    volatile int tiltAngle;
    volatile char alertLevel[8] = "AMAN"; 
    volatile bool stateBinding = false;
    SemaphoreHandle_t xMutex;
    SemaphoreHandle_t mqttMutex;
    SemaphoreHandle_t stringMutex;
public:
    transmitter();
    void init();
    void processBinding();
    void readSensor(bool sireneState, unsigned long sirenePreviousMillis);
    void sendData();
    void wdtReset();
};

