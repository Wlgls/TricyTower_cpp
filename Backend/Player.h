#pragma once

#include <string>
#include "cJSON.h"

enum PlayerEvent{
    NONE = -1,
    DEFAULT = 1,
    //CREATEROOM = 2,
    BEGINGAME = 2,
    INGAME = 3,
};

class Player{
    public:
        Player();
        ~Player();
        void setRoomId(int room_id);
        int roomid() const;

        void setEvent(PlayerEvent event);
        PlayerEvent event() const;

        //void setMsg(cJSON * const msg);
        //cJSON * msg() const;

        void setPlaying(int playing);
        int playing() const;
    
    private:
        int room_id_;
        PlayerEvent event_; // 当前的服务是一个线性的，也就是说当前用户的状态永远都是一个递进的
        //cJSON * msg_; // 当前用户当前状态的信息。
        
        int playing_;
};