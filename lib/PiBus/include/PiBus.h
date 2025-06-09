#ifndef PIBUS_H
#define PIBUS_H

#include <Message.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <cstring>

class PiBus {
public:
    PiBus();
    ~PiBus();
    void openSerial(const char* port, int baud);
    void send(MessageType type, const char* data);
    std::pair<MessageType, std::string> poll();
private:
    int fd;
};

#endif // PIBUS_H