#include "Connection.h"
#include "Epoller.h"
#include "Channel.h"
#include "Player.h"
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <sstream>
#include <iostream>
#include <arpa/inet.h>
#include <openssl/sha.h>
#include <openssl/pem.h>
#include <openssl/bio.h>
#include <openssl/evp.h>

#include "websocket_request.h"
#include "cJSON.h"




Connection::Connection(int clnt_fd, Epoller * ep_){
    channel_ = std::make_unique<Channel> (clnt_fd, ep_);
    // 先握手
    shakehands();

    channel_->set_read_callback(std::bind(&Connection::HandleRead, this));
    channel_->set_write_callback(std::bind(&Connection::HandleWrite, this));
    
    // 初始的时候，先注册一个EPOLLOUT时间，用于向该用户发送room信息。
    channel_->EnableWrite();
    channel_->EnableRead();
}

int Connection::fd(){
    return channel_->fd();
}

Channel * Connection::channel(){
    return channel_.get();
}

void Connection::HandleWrite(){
    //std::cout << "HandleWrite" << std::endl;
    handle_write_(this);
}

void Connection::HandleRead(){
    //std::cout << "HandleRead" << std::endl;
    handle_read_(this);
};

void Connection::set_handle_read(const std::function<void (Connection *)> & cb){
    handle_read_ = cb;
};
void Connection::set_handle_write(const std::function<void (Connection *)>& cb){
    handle_write_ = cb;
};


void Connection::send_head(int payload_length){
    char *response_head;
    int head_length = 0;
    if(payload_length < 126){
        response_head = (char *)malloc(2);
        response_head[0] = 0x81;    
        response_head[1] = payload_length;
        head_length = 2;
    }else if(payload_length < 0xFFFF){
        response_head = (char *) malloc(4);
        response_head[0] = 0x81;
        response_head[1] = 126;
        unsigned short len = htons(payload_length);
        memcpy(response_head + 2, &len, sizeof(len));
        head_length = 4;
    }else {
        response_head = (char *) malloc(12);
        response_head[0] = 0x81;
        response_head[1] = 127;
        unsigned long long len = htobe64(payload_length);
        memcpy(response_head + 2, &len, sizeof(len));
        head_length = 12;
    }
    if (write(channel_->fd(), response_head, head_length) <= 0) {
        perror("socket send head error");
        return;
    }
    free(response_head);
}

void Connection::send_msg(char *msg) {
    int payload_length = strlen(msg);
    // 由于没有buffer，所以先将头送过去
    send_head(payload_length);

    // write paylaod
    if(write(channel_->fd(), msg, payload_length) <= 0){
        perror("socket send msg error");
        return;
    }
}

void Connection::shakehands(){

    std::cout << "shakehand" << std::endl;
    // 一般情况下，都是建立了websocket连接后，才会进行后续的操作，因此在此处不进行监听，而是直接进行读写操作
    char buff[BUFFER_SIZE];
    memset(buff, 0, sizeof(buff));
    // 读取信息
    if(read(channel_->fd(), buff, BUFFER_SIZE) <= 0){
        perror("shakehand read error");
    }

    // 解析信息
    std::istringstream s(buff);

    std::string request;

    /*
    GET / HTTP/1.1\r\n
    Upgrade：websocket\r\n  // 指示客户端希望升级到WebSocket协议
    Connection：Upgrade\r\n //指示客户端希望建立持久连接
    Sec-WebSocket-Key：dGhlIHNhbXBsZSBub25jZQ==\r\n  //生成一个随机的Base64编码密钥，用于安全验证。
    Sec-WebSocket-Version：13\r\n //指示客户端使用的WebSocket协议版本
    \r\n
    */
    // HTTP报文header以\r结束，由于request并不重要，直接忽略

    // response head buffer
    std::string response = "";
    // Sec-WebSocket-Accept
    char sec_accept[32];

    while(std::getline(s, request) && request != "\r"){
        
        if(request.find("Sec-WebSocket-Key") != std::string::npos){
            std::string server_key = request.substr(19, request.size() - 20); // 从第19位开始截取，并且去掉最后的\r
            server_key += GUID;
            unsigned char sha1_data[SHA_DIGEST_LENGTH+1] = {0};

            SHA1((unsigned char *) server_key.c_str(), server_key.size(), (unsigned char *) &sha1_data);

            int len = strlen(reinterpret_cast<const char *> (sha1_data));
            // 方法内部的调用
            base64_encode(reinterpret_cast<char *>(sha1_data), len, sec_accept);
            response += "HTTP/1.1 101 Switching Protocols\r\n";
            response += "Upgrade: websocket\r\n";
            response += "Connection: Upgrade\r\n";
            response += ("Sec-WebSocket-Accept: " + std::string(sec_accept) + "\r\n");
            response += "\r\n";
            if(write(channel_->fd(), response.c_str(), response.size())<0){
                perror("shakehands write error");
            }
            break;
        }
    }
    //std::cout << response << std::endl;
    //std::cout << " end "<< std::endl;
}

void Connection::base64_encode(char *in_str, int in_len, char *out_str){
    BIO *b64, *bio;
    BUF_MEM *bptr = NULL;
    size_t size = 0;

    if (in_str == NULL || out_str == NULL)
        return;

    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new(BIO_s_mem());
    bio = BIO_push(b64, bio);

    BIO_write(bio, in_str, in_len);
    BIO_flush(bio);

    BIO_get_mem_ptr(bio, &bptr);
    memcpy(out_str, bptr->data, bptr->length);
    out_str[bptr->length - 1] = '\0';
    size = bptr->length;

    BIO_free_all(bio);
}