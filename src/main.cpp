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

    Client client{ConnectionManager::getAddress(8080, "127.0.0.1")};

    return 0;
}

