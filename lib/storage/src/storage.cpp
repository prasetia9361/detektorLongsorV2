#include "storage.h"

storage::storage(){}

void storage::init(){
    if (!SPIFFS.begin(true)) {
        Serial.println("Gagal menginisialisasi SPIFFS");
        return;
    }
    
    // Membaca data MAC address dari config.json
    if (SPIFFS.exists("/config.json"))
    {
        JsonDocument doc;
        File file = SPIFFS.open("/config.json",FILE_READ);

        if (!file) {
            Serial.println("Gagal membuka file untuk dibaca");
            memset(configData.macAddress, 0, 6);
            memset(configData.macAddress1, 0, 6);
            return;
        }

        char macStr[128];
        file.readBytes(macStr,file.size());
        file.close();
        
        DeserializationError error = deserializeJson(doc, macStr);
        if (error) {
            Serial.print("deserializeJson() returned ");
        }

        const char* dataMac0 = doc["address0"];
        const char* dataMac1 = doc["address1"];

        for (int i = 0; i < 6; i++) {
            char byteStr[3];
            byteStr[0] = dataMac0[i * 3];
            byteStr[1] = dataMac0[i * 3 + 1];
            byteStr[2] = '\0';
            configData.macAddress[i] = (uint8_t)strtol(byteStr, nullptr, 16);
        }

        for (int i = 0; i < 6; i++) {
            char byteStr[3];
            byteStr[0] = dataMac1[i * 3];
            byteStr[1] = dataMac1[i * 3 + 1];
            byteStr[2] = '\0';
            configData.macAddress1[i] = (uint8_t)strtol(byteStr, nullptr, 16);
        }

        Serial.println("MAC address converted successfully");
    }
    
    // Membaca data WiFi dari wifi.json
    if (SPIFFS.exists("/wifi.json")) {
        JsonDocument wifiDoc;
        File wifiFile = SPIFFS.open("/wifi.json", FILE_READ);
        
        if (!wifiFile) {
            Serial.println("Gagal membuka file wifi.json untuk dibaca");
            setWifi.ssid = "";
            setWifi.pass = "";
            return;
        }
        
        char wifiStr[256];
        wifiFile.readBytes(wifiStr, wifiFile.size());
        wifiFile.close();
        
        DeserializationError wifiError = deserializeJson(wifiDoc, wifiStr);
        if (wifiError) {
            Serial.print("deserializeJson() untuk wifi.json returned ");
            Serial.println(wifiError.c_str());
            setWifi.ssid = "";
            setWifi.pass = "";
            return;
        }
        
        setWifi.ssid = wifiDoc["ssid"].as<String>();
        setWifi.pass = wifiDoc["pass"].as<String>();
        
        Serial.println("Data WiFi berhasil dibaca");
        Serial.print("SSID: ");
        Serial.println(setWifi.ssid);
        Serial.print("Password: ");
        Serial.println(setWifi.pass);
    } else {
        Serial.println("File wifi.json tidak ditemukan");
        setWifi.ssid = "";
        setWifi.pass = "";
    }
}

void storage::writeMacAddress(const uint8_t *mac, int count){
    JsonDocument doc;
    char macStr[18];
    char dataMac[128];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    File file = SPIFFS.open("/config.json",FILE_READ);
    if (!file) {
        Serial.println("Gagal membuka file untuk dibaca");
        memset(configData.macAddress, 0, 6);
        return;
    }
    file.readBytes(dataMac,file.size());
    file.close();
    DeserializationError error = deserializeJson(doc,dataMac);
    if (error) {
        Serial.print("deserializeJson() returned ");
    }

    for (int i = 0; i < count; i++)
    {
        char docName[9];
        sprintf(docName, "address%d", i);
        const char *address = doc[docName];
        if (strlen(address) != 17)
        {
            doc[docName] = macStr;
            File file = SPIFFS.open("/config.json", FILE_WRITE);
            if (!file) {
                Serial.println("- failed to open file for writing");
            }

            serializeJson(doc, file);
            file.close();
            switch (i){
            case 0:
                memcpy(configData.macAddress,mac,6);
                for (int i = 0; i < 6; i++) {
                    Serial.print(configData.macAddress[i], HEX);
                    if (i < 5) {
                        Serial.print(":");
                    }
                }
                Serial.println();
                break;
            case 1:
                memcpy(configData.macAddress1,mac,6);
                for (int i = 0; i < 6; i++) {
                    Serial.print(configData.macAddress1[i], HEX);
                    if (i < 5) {
                        Serial.print(":");
                    }
                }
                Serial.println();
                break;        
            default:
                break;
            }
            break;
        }else
        {
            Serial.print(docName);
            Serial.println(" is available");
        }
    }
}

void storage::writeWifi(const String& ssid, const String& pass){
    File file = SPIFFS.open("/wifi.json", FILE_WRITE);
    if (!file) {
        Serial.println("- gagal membuka file untuk penulisan");
        return;
    }
    
    JsonDocument doc;
    doc["ssid"] = ssid;
    doc["pass"] = pass;
    
    serializeJson(doc, file);
    file.close();
    
    setWifi.ssid = ssid;
    setWifi.pass = pass;
    
    Serial.println("Data WiFi berhasil disimpan");
}

void storage::deleteAddress() {
    File file = SPIFFS.open("/config.json", FILE_WRITE);
    if (!file) {
        Serial.println("- failed to open file for writing");
        return;
    }
    
    JsonDocument doc;
    doc["address0"] = "";
    doc["address1"] = "";
    
    serializeJson(doc, file);
    file.close();
    memset(configData.macAddress, 0, 6);
    memset(configData.macAddress1, 0, 6);
    Serial.println("Addresses cleared successfully");
}