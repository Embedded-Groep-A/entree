#include <PiBus.h>
#include <iostream>
#include <sys/socket.h>

PiBus::PiBus() {}

PiBus::~PiBus() {
    close(fd);
}

void PiBus::openSerial(const char* port, int baud) {
    fd = open(port, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd < 0) {
        perror("open");
        return;
    }

    struct termios options;
    memset(&options, 0, sizeof(options));

    tcgetattr(fd, &options);

    // Set baud rate
    cfsetispeed(&options, baud);
    cfsetospeed(&options, baud);

    // c_cflag: control mode
    options.c_cflag |= (CLOCAL | CREAD); // enable receiver, ignore modem ctrl
    options.c_cflag &= ~PARENB;          // no parity
    options.c_cflag &= ~CSTOPB;          // 1 stop bit
    options.c_cflag &= ~CSIZE;           // clear data bits
    options.c_cflag |= CS8;              // 8 data bits
    options.c_cflag &= ~CRTSCTS;         // disable HW flow control

    // c_lflag: local mode
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG | IEXTEN);

    // c_iflag: input mode
    options.c_iflag &= ~(IXON | IXOFF | IXANY | ICRNL | INLCR | IGNCR | BRKINT | PARMRK | INPCK | ISTRIP);

    // c_oflag: output mode
    options.c_oflag &= ~OPOST;

    // c_cc: control characters
    options.c_cc[VMIN]  = 1;  // read() returns when 1 byte is available
    options.c_cc[VTIME] = 0;  // no timeout

    // Apply settings
    tcflush(fd, TCIFLUSH); // flush input buffer
    tcsetattr(fd, TCSANOW, &options);
}

void PiBus::send(MessageType type, const char* data) {
    std::string message;
    if (type == MessageType::ACCEPT) {
        message = '!' + std::string(data) + '\r';

    } else if (type == MessageType::REJECT) {
        message = std::string("#") + data + "\r";
    } else {
        message = "[" + typeToString(type) + "] " + data;;
    }
    write(fd, message.c_str(), message.size());
    std::cout << "Sent to bus: " << message << std::endl;
}

std::pair<MessageType, std::string> PiBus::poll() {
    static std::string buffer; // Persistent buffer to accumulate data
    char temp[1024] = {0};
    ssize_t bytes = read(fd, temp, sizeof(temp) - 1);
    if (bytes > 0) {
        temp[bytes] = '\0';  // null-terminate
        buffer += temp;      // Append new data to the persistent buffer

        // Check if a complete message is available (ends with '\r')
        size_t endPos = buffer.find('\r');
        if (endPos != std::string::npos) {
            std::string message = buffer.substr(0, endPos); // Extract the message
            buffer.erase(0, endPos + 1);                   // Remove the processed message

            // Parse [TYPE]
            size_t start = message.find('[');
            size_t end = message.find(']');
            if (start != std::string::npos && end != std::string::npos && end > start + 1) {
                std::string typeStr = message.substr(start + 1, end - start - 1);
                MessageType type = stringToType(typeStr);

                std::string data;
                if (end + 1 < message.size()) {
                    data = message.substr(end + 1);
                    // Trim leading space
                    if (!data.empty() && data[0] == ' ') {
                        data = data.substr(1);
                    }
                }

                std::cout << "Parsed message type: " << typeStr << ", data: " << data << std::endl;

                return {type, data};
            }
        }
    }

    return {MessageType::UNKNOWN, ""};
}




