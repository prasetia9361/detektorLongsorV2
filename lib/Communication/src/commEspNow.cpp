#include "commEspNow.h"

#ifdef RECEIVER
const char messaging[12] = "bindingMode";
#else
const char messaging[12] = "bindingSend";
#endif

static commEspNow* instance = NULL;

// Callback untuk receiver
void receiverCallback(const uint8_t* macAddr, const uint8_t* data, int dataLen) {
    // Serial.print("Pesan diterima, panjang: ");
    // Serial.println(dataLen);
    // Serial.print("Data: ");
    // for (int i = 0; i < dataLen; i++) {
    //     Serial.print((char)data[i]);
    // }
    // Serial.println();
    
#ifdef RECEIVER
    if (strcmp((char*)data, "bindingSend") == 0) {
        Serial.println("Menerima pesan binding dari transmitter");
        Serial.print("MAC Address: ");
        for (int i = 0; i < 6; i++) {
            Serial.print(macAddr[i], HEX);
            if (i < 5) Serial.print(":");
        }
        Serial.println();
        instance->memoryStorage->writeMacAddress(macAddr, 1);
        Serial.println("Alamat MAC transmitter berhasil disimpan");
    }
#endif
#ifdef TRANSMITTER
    if (strcmp((char*)data, "bindingMode") == 0){
        Serial.println("Menerima pesan binding dari receiver");
        Serial.print("MAC Address: ");
        for (int i = 0; i < 6; i++) {
            Serial.print(macAddr[i], HEX);
            if (i < 5) Serial.print(":");
        }
        Serial.println();
        instance->memoryStorage->writeMacAddress(macAddr, 1);
        memcpy(instance->dataFromReceiver, data, dataLen);
        Serial.println("Alamat MAC receiver berhasil disimpan");
    }
#endif
    else {
        if (memcmp(macAddr, instance->memoryStorage->getMac(), 6) == 0) {
            memcpy(&instance->messageData, data, sizeof(instance->messageData));
            
            // Konversi char array ke String
            instance->level = String(instance->messageData.data);
        }
    }
}

commEspNow::commEspNow( storage* memoryStorage, uint8_t wifiChannel) {
    instance = this;
    this->memoryStorage = memoryStorage;
    this->wifiChannel = wifiChannel;
    this->bufferSize = maxEspNowPacketSize;
    this->index = 0;
    this->headerSize = 0;
    this->lastData = 0;
}

bool commEspNow::begin() {
    if (memoryStorage == nullptr) {
        Serial.println("Error: memoryStorage tidak terinisialisasi");
        return false;
    }

    // Setup WiFi channel
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_channel(wifiChannel, WIFI_SECOND_CHAN_NONE);
    esp_wifi_set_promiscuous(false);

    // Initialize ESP-NOW
    esp_err_t result = esp_now_init();
    if (result == ESP_OK) {
        Serial.println("ESPNow Init in Receiver Success");
        esp_now_register_recv_cb(receiverCallback);
    } else {
        Serial.printf("ESPNow Init failed: %s\n", esp_err_to_name(result));
        return false;
    }

    return true;
}

void commEspNow::addPeer() {
    if (memoryStorage->getMac()[0] == 0) {
        Serial.println("macAddress nill");
        return;
    }

    esp_now_peer_info_t peerInfo;
    memset(&peerInfo, 0, sizeof(peerInfo));
    memcpy(peerInfo.peer_addr, memoryStorage->getMac(), 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    peerInfo.ifidx = WIFI_IF_STA;

    if (!esp_now_is_peer_exist(memoryStorage->getMac())) {
        esp_now_add_peer(&peerInfo);
    }
}

void commEspNow::sendData(int soil, int angle, const char* level) {
    if (memoryStorage->getMac()[0] == 0) {
        // Serial.println("mac address nill");
        return;
    }

    // Isi data messageData sesuai dengan parameter input
    strncpy(messageData.data, level, sizeof(messageData.data) - 1);
    messageData.data[sizeof(messageData.data) - 1] = '\0'; // Pastikan null-terminated
    messageData.soil = soil;
    messageData.angle = angle;
    Serial.println("sending data");

    esp_now_send(memoryStorage->getMac(), (uint8_t*)&messageData, sizeof(messageData));
}

void commEspNow::statusBinding() {
    esp_now_peer_info_t peerInfo;
    memset(&peerInfo, 0, sizeof(peerInfo));
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    peerInfo.ifidx = WIFI_IF_STA;

    // Set broadcast address
    for (int i = 0; i < 6; i++) {
        peerInfo.peer_addr[i] = 0xFF;
    }

    Serial.println("Memulai binding mode dengan broadcast address");
    if (esp_now_add_peer(&peerInfo) == ESP_OK) {
        Serial.print("Mengirim pesan binding: ");
        Serial.println(messaging);
        
        esp_err_t result = esp_now_send(peerInfo.peer_addr, (uint8_t*)messaging, 12);
        if (result == ESP_OK) {
            Serial.println("Pesan binding berhasil dikirim");
        } else {
            Serial.print("Gagal mengirim pesan binding: ");
            Serial.println(esp_err_to_name(result));
        }
        
        esp_now_del_peer(peerInfo.peer_addr);
    } else {
        Serial.println("Gagal menambahkan peer broadcast");
    }
}