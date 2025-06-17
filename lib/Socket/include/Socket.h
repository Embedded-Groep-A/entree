/**
 * @file Socket.h
 * @brief Definieert de Socket klasse voor netwerkcommunicatie.
 * @author Thijs Kamphuis
 */
#ifndef SOCKET_H
#define SOCKET_H

#include <iostream>
#include <string>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <vector>
#include <unordered_map>
#include <utility>
#include <algorithm>
#include <fcntl.h>

#include "Message.h"


class Socket {
public:
    Socket();
    ~Socket();
    // Singleton
    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;
    Socket(Socket&&) = delete;
    Socket& operator=(Socket&&) = delete;
    /**
     * @brief Maakt een socket aan en bindt deze aan een poort.
     *  
     * @param port de poort waarop de socket luistert
     * @param backlog hoeveel clients er maximaal tegelijk kunnen verbinden
     */
    void host(int port, int backlog);
    /**
     * @brief checkt of er een client verbonden is en retourneert het bericht van de client.
     * 
     * @return ClientMessage het bericht van de client
     */
    ClientMessage poll();
    /**
     * @brief Accepteert een verbinding van een client.
     */
    void accept();
    /**
     * @brief Sluit de socket en alle verbonden clients.
     */
    void close();
    /**
     * @brief Stuur een bericht naar een specifieke client.
     * 
     * @param clientID de ID van de client
     * @param type het type van het bericht
     * @param message de inhoud van het bericht
     */
    void sendToClient(const std::string& clientID, MessageType type, const std::string& message);
    /**
     * @brief Verbind met een socket server.
     * 
     * @param host het IP-adres of de hostnaam van de server
     * @param port de poort waarop de server luistert
     * @param id de ID van deze client
     */
    void connect(const std::string& host, int port, const std::string& id);
    /**
     * @brief Verbreekt de verbinding met de server.
     */
    void disconnect();
    /**
     * @brief Stuur een bericht naar de server.
     * 
     * @param type het type van het bericht
     * @param message de inhoud van het bericht
     */
    void sendToServer(MessageType type, const std::string& message);
    /**
     * @brief Poll de server voor berichten.
     * 
     * @return een pair met het type van het bericht en de inhoud
     */
    std::pair<MessageType, std::string> pollServer();

private:
    int socket_fd;
    std::vector<int> clients;
    std::unordered_map<int, std::string> clientIDs; 

    void disconnectClient(int client_fd);
};


#endif // SOCKET_H