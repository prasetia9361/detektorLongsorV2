#include "receiver.h"
LiquidCrystal_I2C lcd(0x27, 16, 2);
MQTTClient mqttClient;
WiFiClient espClient;
#define ESP_NOW_WIFI_CHANNEL 1
receiver::receiver(){
    memory = new storage();
    comm = new commEspNow(memory, ESP_NOW_WIFI_CHANNEL);
    mButton = new button(BINDING_BUTTON);
    server = new clientServer(memory);
    wifi = new wifiAP();
}

void mqttCallback(String &topic, String &payload) {
  Serial.println("Message received: " + topic + " - " + payload);
  
  if (topic == "nusabot/dapur/lampu") {
    // digitalWrite(LED_RED, payload == "true" ? HIGH : LOW);
  }
}

void receiver::init(){
    // Inisialisasi WDT dengan timeout 10 detik
    if(esp_task_wdt_init(10, true) != ESP_OK) {
        Serial.println("Gagal inisialisasi WDT!");
        while(1);
    }

    memory->init();
    ssid = memory->getSsid();
    pass = memory->getPass();
    
    mqttMutex = xSemaphoreCreateMutex();
    
    bool wifiConnected = false;
    
    // Jika ada kredensial WiFi tersimpan, coba koneksi
    if (ssid != "" && pass != "") {
        Serial.println("Mencoba koneksi dengan kredensial tersimpan");
        wifi->connectToWiFi(ssid, pass);
        
        if (wifi->getMode()) {
            wifiConnected = true;
            // Konek ke MQTT
            mqttClient.begin(MQTT_BROKER, espClient);
            mqttClient.onMessage(mqttCallback);
            Serial.print("Menghubungkan ke MQTT Broker...");
            if (mqttClient.connect("ESP32Client", MQTT_USER, MQTT_PASS)) {
                Serial.println("\nTerhubung ke MQTT!");
                mqttClient.subscribe("nusabot/#");
            } else {
                Serial.println("\nGagal terhubung ke MQTT");
            }
        }
    }
    
    // Jika tidak berhasil terhubung ke WiFi, aktifkan mode AP
    if (!wifiConnected) {
        Serial.println("Mengaktifkan mode Access Point dan Server");
        wifi->setup();
        server->setup();
    }
    
    server->notFound();
    server->init();
    
    if (!comm->begin()) {
        Serial.println("Komunikasi gagal dimulai!");
    }

    // Initialize LCD
    lcd.init();
    lcd.backlight();
    lcd.clear();
    lcd.print("Sistem Monitoring");
    lcd.setCursor(0, 1);
    lcd.print(" Longsor Aktif ");
}

void receiver::processBinding(){
    esp_task_wdt_reset(); 
    
    // Cek apakah ada informasi WiFi baru dari server
    if (server->getConneect()) {
        ssid = memory->getSsid();
        pass = memory->getPass();
        Serial.println("Kredensial WiFi baru diterima, mencoba koneksi");
        
        wifi->disconnectAP();
        wifi->connectToWiFi(ssid, pass);
        
        if (wifi->getMode()) {
            // Konek ke MQTT
            mqttClient.begin(MQTT_BROKER, espClient);
            mqttClient.onMessage(mqttCallback);
            Serial.print("Menghubungkan ke MQTT Broker...");
            if (mqttClient.connect("ESP32Client", MQTT_USER, MQTT_PASS)) {
                Serial.println("\nTerhubung ke MQTT!");
                mqttClient.subscribe("nusabot/#");
            } else {
                Serial.println("\nGagal terhubung ke MQTT");
                // Jika gagal, kembali ke mode AP
                wifi->setup();
            }
        } else {
            // Jika gagal terhubung, kembali ke mode AP
            wifi->setup();
        }
    }
    
    // Cek status koneksi WiFi jika dalam mode station
    if (WiFi.getMode() == WIFI_STA && WiFi.status() != WL_CONNECTED && ssid != "") {
        Serial.println("Koneksi WiFi terputus, mencoba menghubungkan kembali");
        wifi->connectToWiFi(ssid, pass);
        
        if (!wifi->getMode()) {
            // Jika gagal terhubung kembali, aktifkan mode AP
            wifi->setup();
            server->setup();
        }
    }
    
    // proses binding alamat jika tombol ditekan dua kali
    if (mButton->getMode()) {
        comm->statusBinding();
        mButton->setMode(false);
    }
    
    // Proses penghapusan alamat jika tombol long-press ditekan
    if (mButton->getRemove()) {
        memory->deleteAddress(); 
        mButton->setRemove(false); 
    }
}

void receiver::publishData(){
    int moisture = comm->getSoil();
    int angle = comm->getAngle();
    
    // char level[8];
    // strncpy(level, (const char*)comm->getLevel(), sizeof(level));
    // Update LCD
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Status: ");
    lcd.print(comm->getLevel());
    
    lcd.setCursor(0, 1);
    lcd.print("Tanah:");
    lcd.print(moisture);
    lcd.print("% ");
    
    lcd.setCursor(10, 1);
    lcd.print(angle);
    lcd.print((char)223);

    // Publish data with MQTT mutex
    if(xSemaphoreTake(mqttMutex, portMAX_DELAY) == pdTRUE) {
      mqttClient.publish(MQTT_TOPIC_PREFIX + "/soil", String(moisture));
      mqttClient.publish(MQTT_TOPIC_PREFIX + "/mpu", String(angle));
      mqttClient.publish(MQTT_TOPIC_PREFIX + "/level", String(comm->getLevel()));
      xSemaphoreGive(mqttMutex);
    }
    mButton->tick();
}

void receiver::mqttLoop(){
    static bool wdt_registered = false;
    if(!wdt_registered) {
        if(esp_task_wdt_add(NULL) == ESP_OK) {
        wdt_registered = true;
        }
    }
    if(xSemaphoreTake(mqttMutex, portMAX_DELAY) == pdTRUE) {
        esp_task_wdt_reset();  
        mqttClient.loop();
        wifi->loopDns();
    xSemaphoreGive(mqttMutex);
    }
    esp_task_wdt_reset();  // Reset WDT tambahan
    vTaskDelay(1); // Allow lower priority tasks to run
}

void receiver::wdtReset(){
    if(esp_task_wdt_add(NULL) != ESP_OK) {
        Serial.println("Gagal daftarkan Core1 ke WDT!");
    }
}