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

        void setMsg1(cJSON * const msg);
        cJSON * msg1() const;
        void setMsg2(cJSON * const msg);
        cJSON * msg2() const;

        void setPlaying(int playing);
        int playing() const;
    
    private:
        int room_id_;
        PlayerEvent event_; // 当前的服务是一个线性的，也就是说当前用户的状态永远都是一个递进的
        cJSON * msg1_; // 当开始游戏时，需要保存另外两个玩家的信息。
        cJSON * msg2_; 
        int playing_;
};