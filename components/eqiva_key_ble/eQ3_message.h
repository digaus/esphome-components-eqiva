#ifndef DOOR_OPENER_EQ3MESSAGE_H
#define DOOR_OPENER_EQ3MESSAGE_H

#include <time.h>
#include "eQ3_constants.h"

using std::string;

class eQ3Message {
public:
    class MessageFragment {
    public:
        std::string data;
        char getStatusByte();
        int getRemainingFragmentCount();
        bool isFirst();
        bool isLast();
        bool isComplete();
        char getType();
        std::string getData();
        time_t timeSent;
        bool sent = false;
    };

    class Message {
    public:
        char id;
        std::string data;
        Message(std::string data = "");
        virtual std::string encode(ClientState *state);
        bool isSecure();
        static bool isTypeSecure(char type);
        virtual void decode();
    };

    class Connection_Info_Message : public Message {
    public:
        Connection_Info_Message();
        char getUserId();
        std::string getRemoteSessionNonce();
        char getBootloaderVersion();
        char getAppVersion();
    };

    class Status_Changed_Message : public Message {
    public:
        Status_Changed_Message();
    };

    class Status_Info_Message : public Message {
    public:
        Status_Info_Message();
        int getLockStatus();
        bool isBatteryLow();
        int getUserRightType();
    };

    class StatusRequestMessage : public Message {
    public:
        StatusRequestMessage();
        std::string encode(ClientState *state) override;
    };

    class Connection_Close_Message : public Message {
    public:
        Connection_Close_Message();
    };

    class Connection_Request_Message : public Message {
    public:
        Connection_Request_Message();
        std::string encode(ClientState *state) override;
    };

    class CommandMessage : public Message {
    public:
        char command;
        CommandMessage(char command);
        std::string encode(ClientState *state) override;
    };

    class AnswerWithoutSecurityMessage : public Message {
    public:
        AnswerWithoutSecurityMessage();
    };

    class AnswerWithSecurityMessage : public Message {
    public:
        AnswerWithSecurityMessage();
        bool getA();
        bool getB();
    };

    class PairingRequestMessage : public Message {
    public:
        PairingRequestMessage();
        std::string encode(ClientState *state) override;
    };

    class FragmentAckMessage : public MessageFragment {
    public:
        char id;
        FragmentAckMessage(char fragment_id);
    };
};

#endif //DOOR_OPENER_EQ3MESSAGE_H