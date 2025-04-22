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
        trans->processBinding();
        trans->readSensor(sireneState,sirenePreviousMillis);
        vTaskDelay(pdMS_TO_TICKS(100));
    }

}
void Core1TaskCode(void *param){
    trans->wdtReset();
    for (;;)
    {
        trans->sendData();
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
}
#else
#include "receiver/receiver.h"
receiver *reciev;
void applicationTask(void *param);

void setup(){
    Serial.begin(115200);
    reciev = new receiver();
    reciev->init();

    TaskHandle_t taskHandler;
    xTaskCreate(applicationTask, "applicationTask", 10192, NULL, 2, &taskHandler);
}
void loop(){
    reciev->mqttLoop();
}
void applicationTask(void *param){
    reciev->wdtReset();
    bool sireneState = false;
    unsigned long sirenePreviousMillis = 0;
    for (;;){
        reciev->processBinding();
        reciev->publishData();
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
#endif