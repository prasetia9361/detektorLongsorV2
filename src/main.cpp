#ifdef TRANSMITTER
#include "transmitter/transmitter.h"
transmitter *trans;
void Core0TaskCode(void *param);
void Core1TaskCode(void *param);

void setup(){
    Serial.begin(115200);
    trans = new transmitter();
    trans->init();
    // Core Task Handles
    TaskHandle_t Core0Task;
    TaskHandle_t Core1Task;
    // Create tasks
    xTaskCreatePinnedToCore(
        Core0TaskCode,   // Task function
        "Core0Task",     // Task name
        10000,           // Stack size
        NULL,            // Parameters
        1,               // Priority
        &Core0Task,      // Task handle
        0                // Core 0
    );

    xTaskCreatePinnedToCore(
        Core1TaskCode,   // Task function
        "Core1Task",     // Task name
        10000,           // Stack size
        NULL,            // Parameters
        1,               // Priority
        &Core1Task,            // Task handle
        1                // Core 1
    );
}
void loop(){

}
void Core0TaskCode(void *param){
    trans->wdtReset();
    bool sireneState = false;
    unsigned long sirenePreviousMillis = 0;
    for (;;){
        trans->readSensor(sireneState,sirenePreviousMillis);
        // trans->wdtReset();
        vTaskDelay(pdMS_TO_TICKS(100));
    }

}
void Core1TaskCode(void *param){
    trans->wdtReset();
    for (;;)
    {
        trans->processBinding();
        trans->sendData();
        // trans->wdtReset();
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    
}
#else
#include "receiver/receiver.h"
receiver *reciev;
void applicationTask(void *param);
void buttonTask(void *param);

void setup(){
    Serial.begin(115200);
    reciev = new receiver();
    reciev->init();

    TaskHandle_t taskHandler;
    // TaskHandle_t taskButton;
    // xTaskCreate(applicationTask, "applicationTask", 10192, NULL, 1, &taskHandler);
    TaskHandle_t Core0Task;
    // Create tasks
    xTaskCreatePinnedToCore(
        buttonTask,   // Task function
        "buttonTask",     // Task name
        10000,           // Stack size
        NULL,            // Parameters
        1,               // Priority
        &Core0Task,      // Task handle
        0                // Core 0
    );

    xTaskCreatePinnedToCore(
        applicationTask,   // Task function
        "applicationTask",     // Task name
        10000,           // Stack size
        NULL,            // Parameters
        1,               // Priority
        &taskHandler,            // Task handle
        1                // Core 1
    );
    // xTaskCreate(buttonTask, "buttonTask", 10000, NULL, 2, &taskButton);
}
void loop(){
    reciev->mqttLoop();
}
void applicationTask(void *param){
    reciev->wdtReset();
    for (;;){
        reciev->kredensialWifi();
        reciev->publishData();
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

void buttonTask(void *param){
    reciev->wdtReset();
    reciev->beginEspNowandLcd();
    for (;;)
    {
        reciev->printLcd();
        reciev->processBinding();
    }
    vTaskDelay(pdMS_TO_TICKS(10));
    
}
#endif