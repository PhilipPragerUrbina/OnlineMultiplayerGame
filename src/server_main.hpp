//
// Created by Philip on 7/28/2023.
//

#include <iostream>
#include "Networking/UDPSocket.hpp"

int serverMain(){
    std::cout << "Initializing server \n";
    UDPSocket socket(8080);
    std::cout << "Waiting for message from client \n";

    size_t length;
    UDPSocket::Address client_address;
    char* data = socket.receiveFrom(client_address,length);
    std::cout << "Got message from client \n";
    std::string message = std::string(data,length);
    std::cout << "Client sent: " << message << "\n";

    delete[] data;

    std::cout << "Closing server \n";
    socket.close();
    return 0;
}