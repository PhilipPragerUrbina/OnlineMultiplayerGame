//
// Created by Philip on 7/28/2023.
//

#include <iostream>
#include "Networking/ConnectionManager.hpp"

std::vector<u_long> clients;
void recieve(bool TCP, u_long id, const std::vector<uint8_t>& data, ConnectionManager& manager){
    std::string message((const char *)(data.data()));
    if(TCP) {
        std::cout << "TCP received from " << id << " : " << message << "\n";
    }else{
        std::cout << "UDP received from " << id << " : " << message << "\n";
    }
}

void connect( u_long id, ConnectionManager& manager,bool disconnect){
    if(disconnect){
        std::cout << "Client " << id << " disconnected.\n";
        auto position = std::find(clients.begin(), clients.end(), id);
        if (position != clients.end()) {
            clients.erase(position);
        }else{
            std::cout << "Error client disconnect not found!" << "\n";
        }
    }else{
        std::cout << "Client " << id << " connected.\n";
        clients.push_back(id);
    }
}

int serverMain(){
    ConnectionManager server(8080);
    while(true){
        server.processIncoming(&recieve, &connect);
        std::string message;
        std::cout << "Would you like to broadcast a message or 'exit'? \n";
        std::cin >> message;
        if(message == "exit"){
            break;
        }else{
            std::cout << "Preparing for sending: " << message <<   "\n";
            std::vector<uint8_t> data(message.size()+1);
            //copy data
            for (int i = 0; i < message.size()+1; ++i) { //add null terminator
                data[i] = message[i];
            }
            for (const u_long& client : clients) {
                std::cout << "Sending to client with TCP: " << client <<   "\n";
                if(server.writeTCP(client,data)){
                    std::cout << "Sending to client with UDP: " << client <<   "\n";
                    server.writeUDP(client,data);
                };
            }
        }
    }
    std::cout << " Exiting \n";
    return 0;
}
//todo client disconnect freezes
//todo Server disconnect errors the server