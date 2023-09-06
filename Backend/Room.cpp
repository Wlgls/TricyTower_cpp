#include "Room.h"

Room::Room() {}

Room::Room(std::string name, int host) : name_(name), host_(host), player2_(-1), player3_(-1) {
	connectAmount_++;
}

Room::~Room(){
    
}
std::string Room::name() const {
    return name_;
}

void Room::setName(std::string name) {
    name_ = name;
}

int Room::host() const {
    return host_;
}

void Room::setHost(int host) {
    host_ = host;
}

int Room::player2() const {
    return player2_;
}

void Room::setPlayer2(int player2) {
    player2_ = player2;
}

int Room::player3() const {
    return player3_;
}

void Room::setPlayer3(int player3) {
    player3_ = player3;
}

int Room::addPlayer(int player) {
    if (connectAmount() == 3){
        return 0;
    }else if (player2_ == -1) {
        player2_ = player;
        connectAmount_++;
        return 2;
    } else if(player3_ == -1){
        player3_ = player;
        connectAmount_++;
        return 3;
    }
    return 0;
}

int Room::connectAmount() const{
    return connectAmount_;
};

void Room::setConnectAmount(int connectAmount){
    connectAmount_ = connectAmount;
};