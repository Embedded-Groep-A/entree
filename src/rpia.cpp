#include "Socket.h"
#include "PiBus.h"
#include "Message.h"
#include <iostream>
#include <unistd.h>

typedef struct {
    std::string uid;
    const char* eigenaarNaam;
} Eigenaar;

Eigenaar eigenaars[] = {
    {"77 da ac 02", "Cas"},
    {"1e 4c cf 05", "Thijs"},
   // {{0x64, 0x81, 0xE6, 0x03}, "Ahmed"}
};

int main() {
    Socket socket;
    std::string host = "145.52.127.241";
    int port = 8181;
    socket.connect(host, port, "RPIA");

    PiBus piBus;
    piBus.openSerial("/dev/ttyS0", 9600);

    while (true) {
        auto [type, data] = piBus.poll();
        if (type == MessageType::RGB) {
            int r = static_cast<int>(data[0]);
            int g = static_cast<int>(data[1]);
            int b = static_cast<int>(data[2]);
            std::cout << "RGB values: R=" << r << ", G=" << g << ", B=" << b << std::endl;
            std::string rgbString = std::to_string(r) + " " + std::to_string(g) + " " + std::to_string(b);
            socket.sendToServer(MessageType::RGB, rgbString);
        }
        if (type == MessageType::UID) {
            std::string uid(data.begin(), data.end());
            std::cout << "Received UID: " << uid << std::endl;
            bool found = false;
            for (const auto& eigenaar : eigenaars) {
            if (eigenaar.uid == uid) {
                std::cout << "UID matched: " << eigenaar.eigenaarNaam << std::endl;
                const char* eigenaarNaamWithCR = (std::string(eigenaar.eigenaarNaam) + "\r").c_str();
                piBus.send(MessageType::ACCEPT, eigenaarNaamWithCR);
                //piBus.send(MessageType::OPEN, "");
                found = true;
                break;
            }
            }
            if (!found) {
            std::cout << "UID not recognized" << std::endl;
            piBus.send(MessageType::REJECT, "");
            }
        } else if (type == MessageType::BEL) {
            std::cout << "tringelingeling" << std::endl;
        } else if (type == MessageType::STATE) {
            if (data == "SCHEMERLAMP") {
                socket.sendToServer(MessageType::STATE, "SCHEMERLAMP");
            } else if (data == "STOEL") {
                socket.sendToServer(MessageType::STATE, "STOEL");
            }
        }

    }

        


    

    return 0;
}