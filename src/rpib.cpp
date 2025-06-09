#include "Socket.h"
#include <iostream>

int main() {
    Socket socket;
    int port = 8181;
    int backlog = 5;

    socket.host(port, backlog);

    while(true) {
        socket.accept();
        ClientMessage msg = socket.poll();
        if (msg.clientID.empty() || msg.message.empty()) {
            continue;
        }
        // FORWARD RGB VALUE FROM RPIA TO WEMOS
        if (msg.type == MessageType::RGB) {
            socket.sendToClient("WEMOSRGB", MessageType::RGB, msg.message);
        // SEND ON TO RGB WHEN RECEIVNG MOVEMENT  
        } else if (msg.type == MessageType::STATE) {
            if (msg.message == "BEWEGING") {
                socket.sendToClient("WEMOSRGB", MessageType::STATE, "ON");
            } else if (msg.message == "STOEL") {
                socket.sendToClient("WEMOSR", MessageType::STATE, "STOEL");
            }

        }
    }
    return 0;
}   