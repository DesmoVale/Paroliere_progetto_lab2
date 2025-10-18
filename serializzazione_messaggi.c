#include "strutture.h"
#include "serializzazione_messaggi.h"

void deserialize_message(char* buffer, Message* msg) {
    memcpy(&msg -> length, buffer, sizeof(int));
    buffer += sizeof(int);
    memcpy(&msg -> type, buffer, sizeof(char));
    buffer += sizeof(char);
    memcpy(msg -> data, buffer, msg -> length);
}

void serialize_message(Message* msg, char* buffer) {
    memcpy(buffer, &msg -> length, sizeof(int));
    buffer += sizeof(int);
    memcpy(buffer, &msg -> type, sizeof(char));
    buffer += sizeof(char);
    memcpy(buffer, msg -> data, msg -> length);
}