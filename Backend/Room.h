#pragma once

#include<string>

class Room{
    public:
        Room();
        Room(std::string name, int host);
        ~Room();
        int connectAmount() const;

        void setConnectAmount(int connectAmount);

        std::string name() const;

        void setName(std::string name);

        int host() const;

        void setHost(int host);

        int player2() const;

        void setPlayer2(int player2);

        int player3() const;

        void setPlayer3(int player3);

        int addPlayer(int player);
    private:
        std::string name_;
        int connectAmount_ = 0;
        // 三个玩家的ID
        int host_;
        int player2_; 
        int player3_;
};