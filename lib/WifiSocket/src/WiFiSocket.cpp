#include "WiFiSocket.h"
#include <string>

WiFiSocket::WiFiSocket() {

}

WiFiSocket::~WiFiSocket() {}

void WiFiSocket::connectWiFi(const char* ssid, const char* password) {
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi.");
    while(WiFi.status() != WL_CONNECTED) {
      Serial.print(".");
      delay(1000);
    }
    Serial.println();
    Serial.println(WiFi.localIP());
}

void WiFiSocket::connectSocket(const char* host, uint16_t port, const char* id) {
    IPAddress ip;
    WiFi.hostByName(host, ip);
    if (client.connect(ip, port)) {
        Serial.println("Connecting to server at " + String(host) + ":" + String(port));
        client.print(id);
        client.flush();
        Serial.println("Sent ID to server: " + String(id));

        while (!client.available()) {
            delay(100);
        }
        String response = client.readStringUntil('\n');
        if (response == "ACK") {
            Serial.println("Connected to server.");
        }
    } else {
        Serial.println("Failed to connect to server.");
    }
}

void WiFiSocket::disconnectSocket() {
    client.stop();
    Serial.println("Disconnected from server.");
}

void WiFiSocket::sendToServer(MessageType type, const String& message) {
    Serial.println("begin sendToServer");
    String msg = "[" + String(typeToString(type).c_str()) + "] " + message;
    client.println(msg);
    Serial.println("Sent message to server: " + msg);
}

std::pair<MessageType, String> WiFiSocket::pollServer() {
    Serial.println("begin pollServer");
    if (client.available()) {
        String message = client.readStringUntil('\n');
        int pos = message.indexOf(' ');
        if (pos != -1) {
            String typeStr = message.substring(1, pos - 1);
            MessageType type = stringToType(typeStr.c_str());
            message = message.substring(pos + 1);
            Serial.println("Received message from server: [" + typeStr + "] " + message);
            return {type, message};
        } else {
            Serial.println("Invalid message format from server: " + message);
        }
    }
    return {};
}