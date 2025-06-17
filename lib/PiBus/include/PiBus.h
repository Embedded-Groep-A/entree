/**
 * @file PiBus.h
 * @brief Definieert de PiBus klasse voor communicatie via seriële poort.
 * @author Thijs Kamphuis
 */
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
    /**
     * @brief Open een seriële poort voor communicatie.
     * @param port De naam van de seriële poort (bijv. "/dev/ttyUSB0").
     * @param baud De baudrate voor de seriële communicatie (bijv. 9600).
     */
    void openSerial(const char* port, int baud);
    /**
     * @brief Stuur een bericht over de bus.
     * @param type Het type van het bericht.
     * @param data De gegevens van het bericht.
     */
    void send(MessageType type, const char* data);
    /**
     * @brief Check of er berichten beschikbaar zijn op de bus.
     * @return een pair met het type van het bericht en de gegevens.
     */
    std::pair<MessageType, std::string> poll();
private:
    int fd;
};

#endif // PIBUS_H