#ifndef WIFISOCKET_H
#define WIFISOCKET_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "Message.h"

class WiFiSocket {
public:
    WiFiSocket();
    ~WiFiSocket();
    void connectWiFi(const char* ssid, const char* password);
    void connectSocket(const char* host, uint16_t port, const char* id);
    void disconnectSocket();
    void sendToServer(MessageType type, const String& message);
    std::pair<MessageType, String> pollServer();

private:
    WiFiClient client;
    String serverIP;
    uint16_t serverPort;
    
};
#endif // WIFISOCKET_H