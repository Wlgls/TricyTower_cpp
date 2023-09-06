#include "websocket_request.h"
#include <cstring>
#include <iostream>
#include <arpa/inet.h>
WebSocketRequest::WebSocketRequest(char *msg){
    int pos = 0;
    gfin(msg, pos);
    gopcode(msg, pos);
    gmask(msg, pos);
    gpayload_length(msg, pos);
    gmasking_key(msg, pos);
    gpayload(msg, pos);
}

WebSocketRequest::~WebSocketRequest(){}


// 由于unsigned char是一个8bit的，因此需要进行调整
void WebSocketRequest::gfin(char *msg, int &pos){
    fin_ = (unsigned char) msg[pos] >> 7;
}

void WebSocketRequest::gopcode(char *msg, int & pos){
    opcode_ = msg[pos] & 0x0f;
    ++pos; // 第一个8bit结束
}


void WebSocketRequest::gmask(char *msg, int & pos){
    mask_ = (unsigned char) msg[pos] >> 7;
}

void WebSocketRequest::gpayload_length(char *msg, int &pos){
    payload_length_ = msg[pos] & 0x7f;
    pos++;
    if(payload_length_ == 126){
        uint16_t length = 0;
        memcpy(&length, msg+pos, 2);
        pos += 2;
        payload_length_ = ntohs(length);
    }else if(payload_length_ == 127){
        uint32_t length = 0;
        memcpy(&length, msg+pos, 4);
        pos += 4;
        payload_length_ = ntohl(length);
    }
}

void WebSocketRequest::gmasking_key(char *msg, int & pos){
    if(mask_ != 1){
        return;
    }
    memcpy(masking_key_, msg+pos, 4);
    pos += 4;
}

void WebSocketRequest::gpayload(char *msg, int & pos){
    memset(payload_, 0, sizeof(payload_));
    
    memcpy(payload_, msg + pos, payload_length_);
    if(mask_ == 1){
        umask();
    }
    pos += payload_length_;
}

void WebSocketRequest::umask(){
    for(uint i=0; i<payload_length_; ++i){
        payload_[i] ^= masking_key_[i%4];
    }
}

const uint8_t WebSocketRequest::fin() const{
    return fin_;
}
const uint8_t WebSocketRequest::opcode() const{
    return opcode_;
}
const uint8_t WebSocketRequest::mask() const{
    return mask_;
}
const uint8_t * WebSocketRequest::masking_key() const{
    return masking_key_;
}
const uint64_t WebSocketRequest::payload_length() const{
    return payload_length_;
}
const char * WebSocketRequest::payload() const{
    return payload_;
}

