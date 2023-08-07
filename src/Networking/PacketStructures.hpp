//
// Created by Philip on 8/3/2023.
//

#pragma once


#include <cstdint>
#include <cassert>
#include "../Events/EventList.hpp"

//Tick rate in milliseconds
//todo doc more
const uint32_t tick_rate = 15;

/**
 * Additional metadata packaged into outgoing packets that pass game object state.
 */
struct StateMetaData {
    uint8_t buffer_location; //Location in the client array of game-objects
    uint16_t object_id; //unique identifier for this specific object
    uint8_t counter; //Incrementing wrapping counter used to ensure packets arrive in order.
};

/**
  * Additional metadata packaged into outgoing packets that instruct the client to create a new object.
  */
struct NewObjectMetaData {
    uint16_t type_id; //game object type for use with type table.
    uint16_t object_id; //unique identifier for this specific object subtype.
    bool is_associated; //Is this object associated with the current client?
};
//todo instance id: unique identifier for this specific object configuration(Constructor params for re-use).

/**
 * TCP message types
 */
enum TCPMessageType{
    HANDSHAKE, //Client sends initial information to server
    NEW_OBJECT //Server tells a client to create a new object
};

/**
 * States the purpose of a TCP packet
 */
struct MessageTypeMetaData{
    TCPMessageType type;
};

/**
 * Client information
 */
struct HandShake {
    int version;
};

/**
 * Client to a server data message
 */
struct ClientEvents {
    uint8_t counter = 0; //Incrementing wrapping counter used to ensure packets arrive in order.
    EventList list{};
};

/**
 * Append a struct to the end of a packet
 * @tparam T Type.
 * @param packet Current packet data.
 * @param data Data to append.
 */
template <class T> void addStructToPacket(std::vector<uint8_t>& packet, const T& data){
    packet.reserve(packet.size() + sizeof(T));
    for (size_t i = 0; i < sizeof(T); ++i) {
        packet.push_back(((const uint8_t*)&data)[i]); //todo check
    }
}

/**
 * Get a struct from a packet
 * @tparam T Type.
 * @param packet Packet containing struct.
 * @param begin Where the first byte of the struct is within the packet data.
 * @return Struct copied from the packet.
 */
template <class T> T extractStructFromPacket(const std::vector<uint8_t>& packet, size_t begin){
    assert(packet.size() >= sizeof(T));
    T value{};
    memcpy(&value, packet.data()+begin, sizeof(T)); //todo check
    return value;
}