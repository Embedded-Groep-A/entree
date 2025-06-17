/**
 * @file Message.h
 * @brief Definieert de mogelijke berichttypes en de structuur voor berichten.
 * @author Thijs Kamphuis
 */

#ifndef MESSAGE_H
#define MESSAGE_H

#include <string>
#include <cstdint>

/**
 * @enum MessageType
 * @brief Definieert de mogelijke berichttypes die kunnen worden verzonden.
 *
 * - UNKNOWN: Voor onbekende typedata.
 * - RGB: RGB values voor lamp.
 * - ACCEPT: Signaal naar lichtkrant met user.
 * - REJECT: Signaal naar lichtkrant en balie.
 * - OPEN: Signaal naar deur.
 * - BEL: Signaal naar balie.
 * - UID: Signaal naar Pi met UID.
 * - BRAND: Signaal voor deuren en lampen als brand.
 * - STATE: On/Off signaal voor lampen en stoel.
 * - TEXT: Text sturen [testing].
 */

enum class MessageType : uint8_t {
    UNKNOWN = 0,
    RGB = 1,
    ACCEPT = 2,
    REJECT = 3,
    OPEN = 4,
    BEL = 5,
    UID = 6,
    BRAND = 7,
    STATE = 8,
    TEXT = 9,
};

/**
 * @struct ClientMessage
 * @brief Structuur voor berichten die door clients worden verzonden.
 *
 * @var ClientMessage::clientID
 * The unique identifier of the client sending the message.
 * @var ClientMessage::type
 * The type of the message, as defined in MessageType.
 * @var ClientMessage::message
 * The content of the message.
 */
struct ClientMessage {
    std::string clientID;
    MessageType type;
    std::string message;
};


std::string typeToString(MessageType type);
MessageType stringToType(const std::string& str);

#endif // MESSAGE_H