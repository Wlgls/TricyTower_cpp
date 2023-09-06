#include <iostream>
#include <fcntl.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <map>
#include "websocket_request.h"
#include "Room.h"
#include "Player.h"
#include "Channel.h"
#include "Connection.h"
#include "Epoller.h"

class Server{
public:
    Server(int port):listenfd_(-1), room_id_(0){
        ep_ = std::make_unique<Epoller>();
        Create();
        int opt = 1;
        setsockopt(listenfd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof( opt ));
        Bind(port);
        Listen();
        channel_ = std::make_unique<Channel> (listenfd_, ep_.get());
        channel_->set_read_callback(std::bind(&Server::AcceptConnection, this));
        channel_->EnableRead();
    }
    ~Server(){

    }

    void Create(){
        listenfd_ = ::socket(AF_INET, SOCK_STREAM, 0);
        if(listenfd_ == -1){
            perror("Failed to create socket");
        }
    }
    void Bind(int port){
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        //addr.sin_addr.s_addr = htonl(INADDR_ANY);
        addr.sin_addr.s_addr = inet_addr("127.0.0.1");
        addr.sin_port = htons(port);
        if(::bind(listenfd_, (struct sockaddr *)&addr, sizeof(addr))==-1){
            perror("Failed to Bind");
        }
    }
    void Listen(){
        if(::listen(listenfd_, SOMAXCONN) == -1){
            perror("Failed to Listen");
        }
    }

    void AcceptConnection(){
        // 先接收
        std::cout << "Accept An Connection";
        struct sockaddr_in client;
        socklen_t client_addrlength = sizeof(client);
        int clnt_fd = ::accept(listenfd_, (struct sockaddr *)&client, &client_addrlength);

        // 创建相应的连接
        Connection *conn = new Connection(clnt_fd, ep_.get());
        conn->set_handle_write(std::bind(&Server::HandleWrite, this, std::placeholders::_1));
        conn->set_handle_read(std::bind(&Server::HandleRead, this, std::placeholders::_1));
        conn_map_[clnt_fd] = conn;

        // 创建player
        Player player;
        player_map_[clnt_fd] = player;
    };

    void Start(){
        while(true){
            for(Channel *active_ch : ep_->Poll()){
                active_ch->HandleEvent();
            }
        }
    }

    void HandleWrite(Connection *conn){
        //char msg[BUFFER_SIZE];
    
        cJSON *response = cJSON_CreateObject();
        if(player_map_[conn->fd()].event() == PlayerEvent::DEFAULT){
            // 此时用户刚建立，应向其告知当前的房间信息
            
            cJSON_AddNumberToObject(response, "function", 1);
            cJSON_AddNumberToObject(response, "type", 2);
            cJSON_AddNumberToObject(response, "socketId", conn->fd());
            cJSON *data = cJSON_CreateArray();

            for(auto it = room_map_.begin(); it != room_map_.end(); it++){
                if(it->second.connectAmount() == 0){
                    it = room_map_.erase(it);
                    continue;
                }
                cJSON *room = cJSON_CreateObject();
                cJSON_AddNumberToObject(room, "id", it->first);
                cJSON_AddNumberToObject(room, "amount", it->second.connectAmount());
                cJSON_AddStringToObject(room, "name", it->second.name().c_str());
                cJSON_AddNumberToObject(room, "host", it->second.host());
                cJSON_AddNumberToObject(room, "player2", it->second.player2());
                cJSON_AddNumberToObject(room, "player3", it->second.player3());
                cJSON_AddItemToArray(data, room);
            }
            cJSON_AddItemToObject(response, "data", data);
            player_map_[conn->fd()].setEvent(PlayerEvent::NONE); // 修改状态

            conn->send_msg(cJSON_PrintUnformatted(response));
            
        }
        conn->channel()->DisableWrite();
    }

    void HandleRead(Connection * conn){
        //frame_head head;
        //recv_frame_head(conn->fd(), &head);


        char msg[BUFFER_SIZE];
        
        if(read(conn->fd(), msg, sizeof(msg))<=0){
            perror("HandleRead error");
            // 这个关闭显然是存在问题的，应该判断errno的。
            close(conn->fd());
            return;
        }
        // 处理请求，更新本地事务
        // WebSocketRequest request(msg);
        // 这里重复调用构造函数，不断地申请内存空间，感觉性能会存在问题。
        HandleProcess(WebSocketRequest(msg), conn->fd());
    }
    /**/
    void HandleProcess(const WebSocketRequest & request, int clnt_fd){
        cJSON *request_payload = cJSON_Parse(request.payload());

        cJSON *response_payload = cJSON_CreateObject();

        if(!request_payload){
            printf("Error before!");
        }else{
            int function = cJSON_GetObjectItem(request_payload, "function")->valueint;
            switch(function){
                case 1:{
                    std::string name = cJSON_GetStringValue(cJSON_GetObjectItem(cJSON_GetObjectItem(request_payload, "room"), "name"));
                    room_map_[room_id_] = Room(name, clnt_fd);
                    cJSON_AddNumberToObject(response_payload, "function", 1); // 约定该信息为创建成功
                    cJSON_AddNumberToObject(response_payload, "type", 1);
                    cJSON_AddNumberToObject(response_payload, "roomId", room_id_);

                    conn_map_[clnt_fd]->send_msg(cJSON_PrintUnformatted(response_payload));
                    room_id_ ++;
                    // broadcast();
                }
            }
        }

        
    }

private:
    int listenfd_;
    std::unordered_map<int, Room> room_map_;
    std::unordered_map<int, Connection *> conn_map_;
    std::unordered_map<int, Player> player_map_;

    std::unique_ptr<Channel> channel_;
    std::unique_ptr<Epoller> ep_; 

    int room_id_; // 用于分配房间号
};



int main(int argc, char *argv[]){
    int port;
    if (argc <= 1)
    {
        port = 8008;
    }else if (argc == 2){
        port = atoi(argv[1]);
    }else{
        printf("error");
        exit(0);
    }
    Server *server = new Server(port);
    server->Start();
    delete server;
    return 0;

}