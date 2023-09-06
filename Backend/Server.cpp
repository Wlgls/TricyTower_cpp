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
    Server(int port):listenfd_(-1), room_id_(1){
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
        std::cout << "Accept An Connection" << std::endl;
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
    
        
        if(player_map_[conn->fd()].event() == PlayerEvent::DEFAULT){
            // 此时用户刚建立或者是其他需要广播房间信息的时间（如其他用户创建了房间），应向其告知当前的房间信息
            cJSON *response = cJSON_CreateObject();
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

        } else if (player_map_[conn->fd()].event() == PlayerEvent::BEGINGAME){
            cJSON *response = cJSON_CreateObject();
            // 此时用户开始进行游戏，向客户端通知，好，你可以切界面了
            cJSON_AddNumberToObject(response, "function", 2);
            //cJSON_AddNumberToObject(response, "type", 1);
            player_map_[conn->fd()].setEvent(PlayerEvent::NONE); // 每次发送信息后，都将其值none，防止发出了不该发的信息
            conn->send_msg(cJSON_PrintUnformatted(response));

        } else if (player_map_[conn->fd()].event() == PlayerEvent::INGAME){
            cJSON *response1 = cJSON_CreateObject();
            cJSON *response2 = cJSON_CreateObject();
            cJSON_AddNumberToObject(response1, "function", 3);
            cJSON_AddNumberToObject(response2, "function", 3);
            // 信息一个一个发，先发1，再发2
            if (player_map_[conn->fd()].msg1() != nullptr) {
                cJSON_AddNumberToObject(response1, "type", 0); // 左侧
                cJSON_AddItemToObject(response1, "data", player_map_[conn->fd()].msg1());
                player_map_[conn->fd()].setMsg1(nullptr);
            } 
            
            if (player_map_[conn->fd()].msg2() != nullptr) {
                cJSON_AddNumberToObject(response2, "type", 1);
                cJSON_AddItemToObject(response2, "data", player_map_[conn->fd()].msg2());
                player_map_[conn->fd()].setMsg2(nullptr);
            }
            
            // 此时就不用更改状态了
            //conn->send(cJSON_PrintUnformatted(response));
            conn->send_msg(cJSON_PrintUnformatted(response1));
            conn->send_msg(cJSON_PrintUnformatted(response2));
        }
        // 写完之后，将写监听关闭
        //std::cout << "conn_fd: " << conn->fd() << "   " << "response paylaod: " <<cJSON_PrintUnformatted(response) << std::endl;
        // conn->send_msg(cJSON_PrintUnformatted(response1))

        //conn->channel()->EnableRead();
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
        std::cout << "conn fd: " << clnt_fd << "    " << "request payload: " << request.payload() << std::endl;
        cJSON *response_payload = cJSON_CreateObject();
        if(!request_payload){
            printf("Error before!");
        }else{
            int function = cJSON_GetObjectItem(request_payload, "function")->valueint;
            switch(function){
                case 1:{ // 此时某个用户创建了的房间，应向其通知。创建成功
                    std::string name = cJSON_GetStringValue(cJSON_GetObjectItem(cJSON_GetObjectItem(request_payload, "room"), "name"));
                    room_map_[room_id_] = Room(name, clnt_fd);
                    player_map_[clnt_fd].setRoomId(room_id_);
                    cJSON_AddNumberToObject(response_payload, "function", 1); // 约定该信息为创建成功
                    cJSON_AddNumberToObject(response_payload, "type", 1);
                    cJSON_AddNumberToObject(response_payload, "roomId", room_id_);

                    conn_map_[clnt_fd]->send_msg(cJSON_PrintUnformatted(response_payload));
                    room_id_ ++;
                    broadcast();
                    return;
                }
                case 2:{ // 此时客户端告知某用户加入了房间，需更新player和root，并广播
                    int roomId = cJSON_GetObjectItem(cJSON_GetObjectItem(request_payload, "room"), "id")->valueint;
                    room_map_[roomId].addPlayer(clnt_fd);
                    player_map_[clnt_fd].setRoomId(roomId);
                    broadcast();
                    return;
                }
                case 3:{ // 此时客户端开始游戏，服务端更新信息
                    int player_list[3];
                    Player *host = &player_map_[clnt_fd];
                    host->setPlaying(1);
                    player_list[0] = clnt_fd;
                    player_list[1] = room_map_[host->roomid()].player2();
                    player_list[2] = room_map_[host->roomid()].player3();
                    player_map_[player_list[1]].setPlaying(1);
                    player_map_[player_list[2]].setPlaying(1);
                    roomcast(player_list, 3, PlayerEvent::BEGINGAME);
                    return;
                }
                case 4:{ // 当游戏进行时，需要将该用户的信息向当前房间的另外两个玩家公开。
                    int player_list[2];
                    Room currentRoom = room_map_[player_map_[clnt_fd].roomid()];
                    if (currentRoom.host() == clnt_fd) {
                        player_list[0] = currentRoom.player2(); 
                        player_map_[player_list[0]].setMsg1(cJSON_GetObjectItem(request_payload, "bodies")); // 房主
                        player_list[1] = currentRoom.player3();
                        player_map_[player_list[1]].setMsg1(cJSON_GetObjectItem(request_payload, "bodies"));
                    } else if (currentRoom.player2() == clnt_fd) {
                        player_list[0] = currentRoom.host();
                        player_map_[player_list[0]].setMsg1(cJSON_GetObjectItem(request_payload, "bodies"));
                        player_list[1] = currentRoom.player3();
                        std::cout <<  cJSON_GetObjectItem(request_payload, "bodies") << std::endl;
                        player_map_[player_list[1]].setMsg2(cJSON_GetObjectItem(request_payload, "bodies")); // 另一个玩家
                    } else if (currentRoom.player3() == clnt_fd) {
                        player_list[0] = currentRoom.host();
                        player_map_[player_list[0]].setMsg2(cJSON_GetObjectItem(request_payload, "bodies"));
                        player_list[1] = currentRoom.player2();
                        player_map_[player_list[1]].setMsg2(cJSON_GetObjectItem(request_payload, "bodies"));
                    }
                    roomcast(player_list, 2, PlayerEvent::INGAME);
                    return;
                }
            }
        }
    }
    void roomcast(int player_list[], int size,  PlayerEvent ev_){
        for(int i=0; i<size; ++i){
            // 对该房间的用户进行广播，本质上就是根据用户当前状态向客户端发送信息
            player_map_[player_list[i]].setEvent(ev_);
            conn_map_[player_list[i]]->channel()->EnableWrite();
        }
    }
    void broadcast(){
        for(auto &it : player_map_){
            if(it.second.playing() == 1){
                continue;
            }
            it.second.setEvent(PlayerEvent::DEFAULT);
            conn_map_[it.first]->channel()->EnableWrite();
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