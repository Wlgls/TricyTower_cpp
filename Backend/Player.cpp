#include "Player.h"

Player::Player() :
        room_id_(-1), 
        event_(PlayerEvent::DEFAULT),
        //msg_(nullptr),
        playing_(0){};

Player::~Player(){};

void Player::setRoomId(int room_id){
    room_id_ = room_id;
};

int Player::roomid() const{
    return room_id_;
};

void Player::setEvent(PlayerEvent event){
    event_ = event;
};

PlayerEvent Player::event() const{
    return event_;
};

/*
void Player::setMsg(cJSON * const msg){
    msg_ = msg;
};
cJSON *Player::msg() const{
    return msg_;
};
*/

void Player::setPlaying(int playing){
    playing_ = playing;
};

int Player::playing() const{
    return playing_;
};