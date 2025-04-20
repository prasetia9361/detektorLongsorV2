#include <WiFi.h>
#include <WebServer.h>
#include <MQTT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include "HTML.h"

// Pin Definitions
#define SOIL_PIN        36
#define LED_RED         5
#define LED_YELLOW      16
#define LED_GREEN       17
#define SIRENE_PIN      18
#define I2C_SDA         21
#define I2C_SCL         22

// MQTT Configuration
const char* MQTT_BROKER = "privateghofinda.cloud.shiftr.io";
const char* MQTT_USER = "privateghofinda";
const char* MQTT_PASS = "Jqi8J4DAALtisiMk";
String MQTT_TOPIC_PREFIX = "3013042/longsor";

// WiFi AP Configuration
const char* AP_SSID = "ITSNU Pekalongan";
const char* AP_PASS = "12345678";

// Global Objects
LiquidCrystal_I2C lcd(0x27, 16, 2);
Adafruit_MPU6050 mpu;
WebServer server(80);
WiFiClient espClient;
MQTTClient mqttClient;

// Shared Variables with Mutex Protection
volatile int soilMoisture = 0;
volatile int tiltAngle = 0;
String alertLevel = "AMAN";
SemaphoreHandle_t xMutex;
SemaphoreHandle_t mqttMutex;

// Core 0 Task Handle
TaskHandle_t Core0Task;

void setupPins() {
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(SIRENE_PIN, OUTPUT);
  digitalWrite(SIRENE_PIN, LOW);
}

void initWiFiAP() {
  WiFi.softAP(AP_SSID, AP_PASS);
  Serial.print("AP IP Address: ");
  Serial.println(WiFi.softAPIP());
}

void mqttCallback(String &topic, String &payload) {
  Serial.println("Message received: " + topic + " - " + payload);
  
  if (topic == "nusabot/dapur/lampu") {
    digitalWrite(LED_RED, payload == "true" ? HIGH : LOW);
  }
}

void connectToMQTT() {
  mqttClient.begin(MQTT_BROKER, espClient);
  mqttClient.onMessage(mqttCallback);

  Serial.print("Connecting to MQTT Broker...");
  while (!mqttClient.connect("ESP32Client", MQTT_USER, MQTT_PASS)) {
    Serial.print(".");
    delay(1000);
  }
  
  Serial.println("\nConnected to MQTT!");
  mqttClient.subscribe("nusabot/#");
}
void connectToWiFi(const String& ssid, const String& pass) {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), pass.c_str());

  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\nConnected to WiFi!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  
  connectToMQTT();
}

void initWebServer() {
  server.on("/", HTTP_GET, []() {
    server.send_P(200, "text/html", index_html);
  });

  server.on("/action_page", HTTP_POST, []() {
    String newSSID = server.arg("ssidNew");
    String newPass = server.arg("passNew");
    
    server.send_P(200, "text/html", sukses_html);
    delay(2000);
    
    WiFi.softAPdisconnect(true);
    connectToWiFi(newSSID, newPass);
  });
  
  server.begin();
}

// Core 0 Task: Sensor Reading and Alert System
void Core0TaskCode(void *pvParameters) {
  bool sireneState = false;
  unsigned long sirenePreviousMillis = 0;
  
  for(;;) {
    // Read sensors
    int rawValue = analogRead(SOIL_PIN);
    int moisture = map(rawValue, 1024, 454, 0, 100);

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
    String newAlertLevel = "AMAN";
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

    // Update alert level
    if(xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE) {
      alertLevel = newAlertLevel;
      xSemaphoreGive(xMutex);
    }
    
    vTaskDelay(pdMS_TO_TICKS(100)); // Delay for stability
  }
}

// Core 1 Task: LCD Update and Data Publishing
void Core1TaskCode(void *pvParameters) {
  for(;;) {
    int moisture, angle;
    String level;
    
    // Get shared values with mutex
    if(xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE) {
      moisture = soilMoisture;
      angle = tiltAngle;
      level = alertLevel;
      xSemaphoreGive(xMutex);
    }

    // Update LCD
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Status: ");
    lcd.print(level);
    
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
      mqttClient.publish(MQTT_TOPIC_PREFIX + "/level", level);
      xSemaphoreGive(mqttMutex);
    }
    
    vTaskDelay(pdMS_TO_TICKS(2000)); // Update every 2 seconds
  }
}

void setup() {
  Serial.begin(115200);
  Wire.begin(I2C_SDA, I2C_SCL);
  
  // Create mutexes
  xMutex = xSemaphoreCreateMutex();
  mqttMutex = xSemaphoreCreateMutex();

  setupPins();
  initWiFiAP();
  initWebServer();

  if (!mpu.begin()) {
    Serial.println("Failed to initialize MPU6050!");
    while (1);
  }

  // Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.print("Sistem Monitoring");
  lcd.setCursor(0, 1);
  lcd.print(" Longsor Aktif ");
  delay(2000);

  // Create tasks
  xTaskCreatePinnedToCore(
    Core0TaskCode,   /* Task function */
    "Core0Task",     /* Task name */
    10000,           /* Stack size */
    NULL,            /* Parameters */
    1,               /* Priority */
    &Core0Task,      /* Task handle */
    0                /* Core 0 */
  );

  xTaskCreatePinnedToCore(
    Core1TaskCode,   /* Task function */
    "Core1Task",     /* Task name */
    10000,           /* Stack size */
    NULL,            /* Parameters */
    1,               /* Priority */
    NULL,            /* Task handle */
    1                /* Core 1 */
  );
}

void loop() {
  // Main loop runs on Core 1
  server.handleClient();
  
  // Handle MQTT with mutex
  if(xSemaphoreTake(mqttMutex, portMAX_DELAY) == pdTRUE) {
    mqttClient.loop();
    xSemaphoreGive(mqttMutex);
  }
  
  vTaskDelay(1); // Allow lower priority tasks to run
}