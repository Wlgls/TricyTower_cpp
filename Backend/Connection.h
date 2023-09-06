#pragma once
#include <memory>
#include <fcntl.h>
#include <functional>

#define RESPONSE_HEADER_LEN_MAX 1024
#define BUFFER_SIZE 2048
#define GUID "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"

class Player;
class Epoller;
class Channel;
class Connection{

    public:
        Connection(int clnt_fd, Epoller *ep_);
        ~Connection();
        int fd();
        Channel *channel();

        void HandleRead();
        void HandleWrite();

        void set_handle_read(const std::function<void (Connection *)> & cb);
        void set_handle_write(const std::function<void (Connection *)>& cb);

        void set_process_quest(const std::function<void()> &cb);
        //void set_handle_event(const std::function<char *(int)> &cb);

        void send_msg(char *str);
    private:
        void base64_encode(char *in_str, int in_len, char *out_str);

        void send_head(int payload_length);
        

        void shakehands();
        std::unique_ptr<Channel> channel_;

        std::function<void()> process_request;

        std::function<void (Connection *)> handle_read_;
        std::function<void (Connection *)> handle_write_;

};