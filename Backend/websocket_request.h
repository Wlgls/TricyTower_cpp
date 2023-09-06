#pragma once
#include <stdint.h>

/*
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-------+-+-------------+-------------------------------+
|F|R|R|R| opcode|M| Payload len |    Extended payload length    |
|I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |
|N|V|V|V|       |S|             |   (if payload len==126/127)   |
| |1|2|3|       |K|             |                               |
+-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
|     Extended payload length continued, if payload len == 127  |
+ - - - - - - - - - - - - - - - +-------------------------------+
|                               |Masking-key, if MASK set to 1  |
+-------------------------------+-------------------------------+
| Masking-key (continued)       |          Payload Data         |
+-------------------------------- - - - - - - - - - - - - - - - +
:                     Payload Data continued ...                :
+ - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
|                     Payload Data continued ...                |
+---------------------------------------------------------------+
--------------------------------------------------------------------*/



class WebSocketRequest{
    public:
        WebSocketRequest(char *msg);
        ~WebSocketRequest();
        const uint8_t fin() const;
        const uint8_t opcode() const;
        const uint8_t mask() const;
        const uint8_t * masking_key() const;
        const uint64_t payload_length() const;
        const char * payload() const;

    
    private:
        void gfin(char *msg, int &pos);
        void gopcode(char *msg, int &pos);
        void gmask(char *msg, int &pos);
        void gpayload_length(char *msg, int &pos);
        void gmasking_key(char *msg, int &pos);
        void gpayload(char *msg, int &pos);

        void umask();
    
        uint8_t fin_;
        uint8_t opcode_;
        uint8_t mask_;
        uint8_t masking_key_[4];
        uint64_t payload_length_;
        char payload_[2048];
};