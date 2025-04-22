#ifndef WIFIAP_H
#define WIFIAP_H
#include <DNSServer.h>
#include <WiFi.h>
class wifiAP {
   private:
    DNSServer dnsServer;
    const char *ssid = "ESP32-Remote";
    const char *password = "project123";

    const char *ssid1 = "ESP32-Remote";
    const char *password1 = "12345678";
    bool mode = false;
    String macAddress = WiFi.macAddress();

   public:
    // void begin();
    wifiAP();
    // void begin();
    void setup();
    void loopDns();
    void connectToWiFi(const String& ssid, const String& pass);
    // String getMac() { return macAddress; }
    void disconnectAP();
    bool getMode(){return mode;}
};
#endif
