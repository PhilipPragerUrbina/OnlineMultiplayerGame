#include "server_main.hpp"
#include "Client.hpp"
//todo add external licenses to external folder

//for now any additional command line args create server
int main(int argc, char* argv[]) {

    bool is_server = false;
    if(argc > 1){
        is_server = true;
    }
    if(is_server){
        return serverMain();
    }

    start:
    std::cout << "Enter ip address or 'L' for local host\n";
    std::string ip;
    std::cin >> ip;
    if(ip == "L"){
        ip = "127.0.0.1";
    }

    try {
        Client client{ConnectionManager::getAddress(8080, ip)};
    } catch (const std::runtime_error& error){
        std::cout << error.what() << "\n";
        goto start;
    }
    return 0;
}

