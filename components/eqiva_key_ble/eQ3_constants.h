#ifndef DOOR_OPENER_CONST_H
#define DOOR_OPENER_CONST_H

#include <string>

typedef enum {
    DISCONNECTED = 0,
    SCANNING = 1,
    FOUND = 2,
    CONNECTING = 3,
    CONNECTED = 4,
    NONCES_EXCHANGED = 5
} ConnectionState;

typedef struct {
    char user_id;
    std::string user_key;
    std::string local_session_nonce;
    std::string remote_session_nonce;
    uint16_t local_security_counter = 1;
    uint16_t remote_security_counter = 0;
    ConnectionState connectionState = DISCONNECTED;
} ClientState;

typedef enum {
    LOCK = 0,
    UNLOCK = 1,
    OPEN = 2,
    REQUEST_STATUS = 3
} CommandType;

typedef enum {
    UNKNOWN = 0,
    MOVING = 1,
    UNLOCKED = 2,
    LOCKED = 3,
    OPENED = 4
} LockStatus;

#endif //DOOR_OPENER_CONST_H