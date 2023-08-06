//
// Created by Philip on 7/28/2023.
//

#include "Server.hpp"

int serverMain(){
    Server server(8080);
    std::cout << "Started server on port: " << 8080 << "\n";
    while (true){
       // std::cout << "running \n";
    }
    //todo shut down server on command
    //todo add console to type commands
    //todo add server view
    //todo add asset streaming(send new content like custom player models directly to client), and https server to look up player info from database
    return 0;
}
