#include "wifiAP.h"

wifiAP::wifiAP() {}
// void wifiAP::begin() {
//     IPAddress apIP(192, 168, 7, 1);
//     IPAddress subNet(255, 255, 255, 0);
//     WiFi.mode(WIFI_MODE_STA);
//     WiFi.softAPConfig(apIP, apIP, subNet);
//     // WiFi.softAP(ssid, password);
//     // Serial.print("IP address: ");
//     // IP = WiFi.softAPIP();
//     // Serial.println(IP);
//     Serial.println(WiFi.macAddress());
//     WiFi.softAP(ssid, password, 6, 0, 4);

//     dnsServer.setTTL(3600);
//     dnsServer.start(53, "*", apIP);
//     // while (WiFi.status() != WL_CONNECTED) {
//     //     Serial.print(".");
//     //     delay(1000);
//     // }
// }

void wifiAP::setup() {
    WiFi.disconnect();
    WiFi.mode(WIFI_AP);
    
    IPAddress apIP(192, 168, 7, 2);
    IPAddress subNet(255, 255, 255, 0);
    WiFi.softAPConfig(apIP, apIP, subNet);
    Serial.println(macAddress);
    WiFi.softAP(ssid1, password1, 6, 0, 4);
    
    dnsServer.setTTL(3600);
    dnsServer.start(53, "*", apIP);
    Serial.println("Mode AP aktif dengan IP: " + WiFi.softAPIP().toString());
}

void wifiAP::connectToWiFi(const String& ssid, const String& pass){
    WiFi.disconnect();
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), pass.c_str());

    Serial.print("Menghubungkan ke WiFi");
    int retry = 20; // 10 detik timeout
    while (WiFi.status() != WL_CONNECTED && retry > 0) {
        delay(500);
        Serial.print(".");
        retry--;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        mode = true;
        Serial.println("\nTerhubung ke WiFi. IP: " + WiFi.localIP().toString());
    } else {
        mode = false;
        Serial.println("\nGagal terhubung ke WiFi");
    }
}

void wifiAP::disconnectAP(){
    if (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA) {
        WiFi.softAPdisconnect(true);
        Serial.println("Access Point dinonaktifkan");
    }
}

void wifiAP::loopDns() {
    if (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA) {
        dnsServer.processNextRequest();
    }
}
