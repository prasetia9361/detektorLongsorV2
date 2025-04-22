#include "transmitter.h"

Adafruit_MPU6050 mpu;

#define ESP_NOW_WIFI_CHANNEL 1
transmitter::transmitter(){
    soilMoisture = 0;
    tiltAngle = 0;
    memory = new storage();
    comEspnow = new commEspNow(memory, ESP_NOW_WIFI_CHANNEL);
    mButton = new button(BINDING_BUTTON);
}

void transmitter::init(){
  if(esp_task_wdt_init(10, true) != ESP_OK) {
    Serial.println("Gagal inisialisasi WDT!");
    while(1);
  }
  memory->init();
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  Serial.print("My MAC Address is: ");
  Serial.println(WiFi.macAddress());
  if (!comEspnow->begin()) {
      Serial.println("Komunikasi gagal dimulai!");
  }

  Wire.begin(I2C_SDA, I2C_SCL);
  
  // Create mutexes
  xMutex = xSemaphoreCreateMutex();
  mqttMutex = xSemaphoreCreateMutex();
  stringMutex = xSemaphoreCreateMutex();

  pinMode(LED_RED, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(SIRENE_PIN, OUTPUT);
  digitalWrite(SIRENE_PIN, LOW);

  mButton->begin();

  if (!mpu.begin()) {
  Serial.println("Failed to initialize MPU6050!");
  while (1);
  }

  comEspnow->addPeer();
}

void transmitter::processBinding(){
  esp_task_wdt_reset(); 
  // proses binding alamat jika tombol ditekan dua kali
  if (mButton->getMode())
  {
      comEspnow->statusBinding();
      mButton->setMode(false);
  }
  
  //Proses penghapusan alamat jika tombol long-press ditekan
  if (mButton->getRemove()) 
  {
      memory->deleteAddress(); 
      mButton->setRemove(false); 
  }
}

void transmitter::readSensor(bool sireneState, unsigned long sirenePreviousMillis){
    // Read soil moisture
    int rawValue = analogRead(SOIL_PIN);
    int moisture = map(rawValue, 1024, 454, 0, 100);

    // Read MPU6050
    sensors_event_t accel;
    mpu.getAccelerometerSensor()->getEvent(&accel);
    float x = accel.acceleration.x;
    float y = accel.acceleration.y;
    float z = accel.acceleration.z;
    int angle = (atan2(sqrt(x*x + y*y), z) * 180.0) / PI;

    // Update shared variables with mutex
    if(xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE) {
      soilMoisture = moisture;
      tiltAngle = angle;
      xSemaphoreGive(xMutex);
    }

    // Update alert system
    const char* newAlertLevel;
    if (moisture >= 80 || angle > 30) {
      newAlertLevel = "BAHAYA";
      digitalWrite(LED_RED, HIGH);
      digitalWrite(LED_YELLOW, LOW);
      digitalWrite(LED_GREEN, LOW);
      
      // Blink sirene
      unsigned long currentMillis = millis();
      if (currentMillis - sirenePreviousMillis >= 1000) {
        sireneState = !sireneState;
        digitalWrite(SIRENE_PIN, sireneState);
        sirenePreviousMillis = currentMillis;
      }
    }
    else if ((moisture >= 50 && moisture < 80) || (angle > 15 && angle <= 30)) {
      newAlertLevel = "WASPADA";
      digitalWrite(LED_YELLOW, HIGH);
      digitalWrite(LED_RED, LOW);
      digitalWrite(LED_GREEN, LOW);
      digitalWrite(SIRENE_PIN, LOW);
    }
    else {
      newAlertLevel = "AMAN";
      digitalWrite(LED_GREEN, HIGH);
      digitalWrite(LED_RED, LOW);
      digitalWrite(LED_YELLOW, LOW);
      digitalWrite(SIRENE_PIN, LOW);
    }

    // Update alert level with mutex
    if(xSemaphoreTake(stringMutex, portMAX_DELAY) == pdTRUE) {
      strncpy((char*)alertLevel, newAlertLevel, sizeof(alertLevel)-1);
      alertLevel[sizeof(alertLevel)-1] = '\0';
      xSemaphoreGive(stringMutex);
    }
    mButton->tick();
}

void transmitter::sendData(){
    int moisture, angle;
    char level[8];
    
    // Get shared values with mutex
    if(xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE) {
        moisture = soilMoisture;
        angle = tiltAngle;
        xSemaphoreGive(xMutex);
    }

    // Get alert level with mutex
    if(xSemaphoreTake(stringMutex, portMAX_DELAY) == pdTRUE) {
        strncpy(level, (const char*)alertLevel, sizeof(level));
        xSemaphoreGive(stringMutex);
    }

    static unsigned long lastSendTime = 0;
    unsigned long currentTime = millis();
    
    // Kirim data setiap 5 detik sekali
    // if (currentTime - lastSendTime >= 5000) {
    
    comEspnow->sendData(moisture, angle, level);
        
        // Perbarui waktu pengiriman terakhir
    //     lastSendTime = currentTime;
    // }
}

void transmitter::wdtReset(){
    if(esp_task_wdt_add(NULL) != ESP_OK) {
        Serial.println("Gagal daftarkan Core1 ke WDT!");
    }
}