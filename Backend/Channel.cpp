#include "Channel.h"
#include "Epoller.h"
#include <sys/epoll.h>
#include <unistd.h>
#include <utility>


Channel::Channel(int fd, Epoller * ep) : fd_(fd), ep_(ep),
                                            listen_events_(0), ready_events_(0),
                                            in_epoll_(false){};

Channel::~Channel() { 
}


void Channel::HandleEvent() const{
    if (ready_events_ & EPOLLIN ) {
        if (read_callback_) {
            read_callback_();
        }
    } 

    if (ready_events_ & EPOLLOUT) {
        if (write_callback_) {
            write_callback_();
        }
    }
}

void Channel::EnableRead(){
    listen_events_ |= EPOLLIN;
    ep_->UpdateChannel(this);
}

void Channel::EnableWrite(){
    listen_events_ |= EPOLLOUT;
    ep_->UpdateChannel(this);
}

void Channel::DisableWrite(){
    listen_events_ &= ~EPOLLOUT;
    ep_->UpdateChannel(this);
}

/*
void Channel::OnlyEnableWrite(){
    if(listen_events_ & EPOLLIN){
        listen_events_ |= ~EPOLLIN;
    }
    listen_events_ |= EPOLLOUT;
    ep_->UpdateChannel(this);
}

void Channel::OnlyEnableWrite(){
    if(listen_events_ & EPOLLOUT){
        listen_events_ |= ~EPOLLOUT;
    }
    listen_events_ |= EPOLLIN;
    ep_->UpdateChannel(this);
}

void Channel::DisableWrite(){
    listen_events_ |= ~EPOLLOUT;
    ep_->UpdateChannel(this);
}

void Channel::DisableRead(){
    listen_events_ |= ~EPOLLIN;
    ep_->UpdateChannel(this);
}

*/

int Channel::fd() const { return fd_; }

short Channel::listen_events() const { return listen_events_; }
short Channel::ready_events() const { return ready_events_; }

bool Channel::IsInEpoll() const { return in_epoll_; }
void Channel::SetInEpoll(bool in) { in_epoll_ = in; }

void Channel::SetReadyEvents(int ev){
    ready_events_ = ev;
}

void Channel::set_read_callback(std::function<void()> const &callback) { read_callback_ = std::move(callback); }
void Channel::set_write_callback(std::function<void()> const &callback) { write_callback_ = std::move(callback); }