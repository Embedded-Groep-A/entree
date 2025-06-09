#include "Socket.h"

Socket::Socket() {
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
}

Socket::~Socket() {
    close();
}

void Socket::host(int port, int backlog) {
    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    bind(socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));

    listen(socket_fd, backlog);

    std::cout << "Server is listening on port " << port << std::endl;

    accept();
}

void Socket::accept() {
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(socket_fd, &read_fds);

    timeval timeout{};
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;

    if (select(socket_fd + 1, &read_fds, nullptr, nullptr, &timeout) > 0 &&
        FD_ISSET(socket_fd, &read_fds)) {
        
        sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        int client_fd = ::accept(socket_fd, (struct sockaddr*)&client_addr, &addr_len);

        if (client_fd >= 0) {
            char ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &client_addr.sin_addr, ip, sizeof(ip));
            std::cout << "Client connecting from " << ip << std::endl;

            char ID[9] = {0};
            int bytes_received = recv(client_fd, ID, 8, 0);
            if (bytes_received > 0) {
                ID[bytes_received] = '\0';
                std::string idStr(ID);
                std::cout << "Received ID: " << idStr << std::endl;

                clientIDs[client_fd] = idStr;
                send(client_fd, "ACK", 3, 0);
                clients.push_back(client_fd);

            } else {
                ::close(client_fd);
            }
        }
    }
}


ClientMessage Socket::poll() {
    fd_set read_fds;
    FD_ZERO(&read_fds);
    int max_fd = -1;

    for (int fd : clients) {
        if (fd >= 0) {
            FD_SET(fd, &read_fds);
            if (fd > max_fd) max_fd = fd;
        }
    }

    if (max_fd < 0) {
        return {};
    }

    timeval timeout{};
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;

    int ready = select(max_fd + 1, &read_fds, nullptr, nullptr, &timeout);
    if (ready <= 0) {
        return {};
    }

    ClientMessage result;

    for (auto client = clients.begin(); client != clients.end(); ) {
        int fd = *client;

        if (FD_ISSET(fd, &read_fds)) {
            char buffer[1024];
            ssize_t bytes = recv(fd, buffer, sizeof(buffer), 0);

            if (bytes <= 0) {
                std::string clientID = clientIDs[fd];
                client = clients.erase(client);
                disconnectClient(fd);
                continue;
            } else {
                std::string message(buffer, bytes);
                std::string clientID = clientIDs[fd];

                size_t pos = message.find(' ');
                if (pos != std::string::npos) {
                    std::string typeStr = message.substr(1, pos - 2);
                    MessageType type = stringToType(typeStr);
                    message = message.substr(pos + 1);
                    result = {clientID, type, message};
                    std::cout << "Received message from client " << clientID << ": [" << typeStr << "] " << message << std::endl;
                }
            }
        }
        ++client;
    }

    return result;
}




void Socket::close() {
    ::close(socket_fd);
    socket_fd = -1;
    std::cout << "Socket closed." << std::endl;
}

void Socket::sendToClient(const std::string& clientID, MessageType type, const std::string& rawMsg) {
    std::string message = "[" + typeToString(type) + "] " + rawMsg;

    for (const auto& pair : clientIDs) {
        if (pair.second == clientID) {
            int client_fd = pair.first;
            send(client_fd, message.c_str(), message.size(), 0);
            std::cout << "Sent message to client " << clientID << ": " << message << std::endl;
            return;
        }
    }

    std::cout << "Client ID not found: " << clientID << std::endl;
}

void Socket::connect(const std::string& host, int port, const std::string& id) {
    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, host.c_str(), &server_addr.sin_addr);

    ::connect(socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));

    std::cout << "Connecting to server at " << host << ":" << port << std::endl;
    // Send ID to server
    send(socket_fd, id.c_str(), id.size(), 0);
    // Wait for ACK
    char buffer[1024];
    int bytes_received = recv(socket_fd, buffer, sizeof(buffer), 0);
    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';
        if (strcmp(buffer, "ACK") == 0) {
            std::cout << "Connected to server." << std::endl;
        }
    }
}

void Socket::disconnect() {
    close();
    std::cout << "Disconnected from server." << std::endl;
}

void Socket::sendToServer(MessageType type, const std::string& rawMsg) {
    std::string message = "[" + typeToString(type) + "] " + rawMsg;
    send(socket_fd, message.c_str(), message.size(), 0);
    std::cout << "Sent message to server: " << message << std::endl;
}

std::pair<MessageType, std::string> Socket::pollServer() {
    char buffer[1024];
    ssize_t bytes = recv(socket_fd, buffer, sizeof(buffer), 0);

    if (bytes > 0) {
        std::string message(buffer, bytes);
        
        size_t pos = message.find(' ');
        if (pos != std::string::npos) {
            std::string typeStr = message.substr(1, pos - 2);
            MessageType type = stringToType(typeStr);
            message = message.substr(pos + 1);
            std::cout << "Received message from server: [" << typeStr << "] " << message << std::endl;
            return {type, message};
        } else {
            std::cout << "Invalid message format from server: " << message << std::endl;
        }
    }
    return {};
}

void Socket::disconnectClient(int client_fd) {
    ::close(client_fd);
    std::string clientID = clientIDs[client_fd];
    clientIDs.erase(client_fd);
    std::cout << "Client " << clientID << " disconnected." << std::endl;
}