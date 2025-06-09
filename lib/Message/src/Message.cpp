/**
 * @file Message.cpp
 * @brief Sourcecode voor de Message functies.
 * @author Thijs Kamphuis
 * 
 */


 

#include "Message.h"

/**
 * @brief Zet de MessageType enum waarde om naar een string.
 *
 * @param type De MessageType.
 * @return Een stringrepresentatie van de MessageType.
 */
std::string typeToString(MessageType type) {
    switch (type) {
        case MessageType::RGB: return "RGB";
        case MessageType::ACCEPT: return "ACCEPT";
        case MessageType::REJECT: return "REJECT";
        case MessageType::OPEN: return "OPEN";
        case MessageType::BEL: return "BEL";
        case MessageType::UID: return "UID";
        case MessageType::BRAND: return "BRAND";
        case MessageType::STATE: return "STATE";
        case MessageType::TEXT: return "TEXT";
        default: return "UNKNOWN";
    }
}
/**
 * @brief Zet een string om naar de bijbehorende MessageType.
 *
 * @param str De string.
 * @return De bijbehorende MessageType.
 */
MessageType stringToType(const std::string& str) {
    if (str == "RGB") return MessageType::RGB;
    if (str == "ACCEPT") return MessageType::ACCEPT;
    if (str == "REJECT") return MessageType::REJECT;
    if (str == "OPEN") return MessageType::OPEN;
    if (str == "BEL") return MessageType::BEL;
    if (str == "UID") return MessageType::UID;
    if (str == "BRAND") return MessageType::BRAND;
    if (str == "STATE") return MessageType::STATE;
    if (str == "TEXT") return MessageType::TEXT;
    return MessageType::UNKNOWN;
}