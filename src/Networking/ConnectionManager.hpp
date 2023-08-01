//
// Created by Philip on 7/29/2023.
//

#pragma once

#include <stdexcept>

#include <winsock2.h>
#include <Ws2tcpip.h>
#include <vector>
#include <cassert>

/**
 * Use this class to connect to a server or to wait for and connect to multiple clients as a server
 * Allows a combination of TCP and UDP packets to be sent once connected
 * Allows single threaded management of child sockets(Connections between a single client and the server)
 * Works using WINSOCK or POSIX
 * Multiple connection managers can be run at the same time.
 */
class ConnectionManager {
private:
    /**
     * Manage global winsock initialization and clean up using static constructors and destructors
     * @details This can also just be done once per connection manager instance, as winsock has an internal counter.
     * Nevertheless, this ensures that winsock commands are valid for the entire lifetime of the program, just like POSIX.
     * @see https://learn.microsoft.com/en-us/windows/win32/api/winsock/nf-winsock-wsastartup
     */
    struct GlobalWinsock {
        /**
          * Start up winsock version 2.2 globally.
          * @throw runtime_error Error starting winsock
          */
        GlobalWinsock(){
            WSADATA winsock_data; //Not needed
            if(WSAStartup(MAKEWORD(2,2), &winsock_data) !=0 ) throw std::runtime_error("Error starting winsock.");
        }

        /**
          * Clean up winsock globally
          * @throw runtime_error Error closing winsock
          */
        ~GlobalWinsock(){
            if(WSACleanup() != 0) throw std::runtime_error("Error closing winsock.");
        }
    };
    inline static GlobalWinsock global_winsock{};
public:
    /**
     * Represents a port number
     */
    typedef unsigned short Port;
    /**
     * Represents a connection address
     */
    typedef sockaddr_in  Address;
    /**
     * Represents a socket ID
     */
    typedef SOCKET Socket;
private:
    /**
     * Represents a single server-client connection
     */
    struct Connection {
        Socket child_socket_tcp; //File descriptor for TCP connection
        Address other_address; //Address of the other device
    };
    std::vector<Connection> active_connections;
    fd_set watching_connections;


    Socket main_socket; //Used for creating connections
    Socket data_socket; //UDP data socket
    bool closed = false; //Is the socket still open?
    bool server = false; //Is this a server or a client?
public:

    /**
     * Create a client socket that can connect to a server.
     * Is not bound to a particular port.
     * @throw runtime_error Error starting socket
     */
    ConnectionManager() {
        //Create socket
        main_socket = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
        if(main_socket == INVALID_SOCKET)  throw std::runtime_error("Error starting main socket.");
        data_socket = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
        if(data_socket == INVALID_SOCKET)  throw std::runtime_error("Error starting data socket.");
    }

    /**
     * Create a server socket bound to a port
     * @param port Port to bind to
     * @throw runtime_error Error starting socket
     * Uses any IP address it can.
     * It will start listening right away.
     */
    ConnectionManager(Port port) : ConnectionManager() { //Create socket
        server = true;
        //Set socket mode
        int opt = 1;
        if(setsockopt(main_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt,sizeof(opt)) < 0) throw std::runtime_error("Error setting socket options.");
        //Bind socket
        sockaddr_in server_options{};
        server_options.sin_port = htons(port);
        server_options.sin_family = AF_INET;
        server_options.sin_addr.s_addr = htonl(INADDR_ANY);
        if(bind(main_socket, (SOCKADDR *) &server_options, sizeof(server_options)) != 0) throw std::runtime_error("Error binding main socket.");
        if(bind(data_socket, (SOCKADDR *) &server_options, sizeof(server_options)) != 0) throw std::runtime_error("Error binding data socket.");

        if(listen(main_socket,10) !=0 ) { throw std::runtime_error("Error listening."); }; //Set listen backlogs

    }

    /**
      * Get the address struct from address information
      * @param port Address port
      * @param ip_address IPV4 IP address in string format
      * @return Address struct
      */
    static Address getAddress(Port port,const std::string& ip_address){
        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_port = htons(port);
        address.sin_addr.s_addr = inet_addr(ip_address.c_str());
        return address;
    }

    /**
     * Process an incoming message
     * Will accept new connections, and will process UDP and TCP message
     * Only does one
     * Will not do anything if the manager is already closed or not a server
     */

    /**
     * Process all currently incoming packets
     * Will accept new connections, and will process UDP and TCP message
     * Will not do anything if the manager is already closed or not a server
     * @param receive_callback Callback for incoming UDP and TCP packets.
     * @details TCP boolean is true if TCP false is UDP. Client ID is any integer unique to the client. The packet data is a copy of the data sent.
     * The manager is a reference to this such that a response can be sent right away if needed.
     * @param connection_callback Callback for new connection or disconnect
     * @param timeout_ms Time period to wait for packets in milliseconds. Useful if application has a tick rate.
     * @param max_packets The maximum number of simultaneous or consecutive UDP packets that would be expected to arrive within the timeout. Just used as an upper bound for how many times to select.
     */
    void processIncoming(void (*receive_callback)(bool TCP, int client_id, const std::vector<uint8_t>& packet_data,ConnectionManager& manager),void (*connection_callback)(int client_id,ConnectionManager& manager,bool disconnect),
                         int timeout_ms = 50, int max_packets = 20){
        if(closed || !server) return;
        //Collect multiple messages from one socket if needed
        for (int i = 0; i < max_packets; ++i) {
            //Add connections
            Socket max_socket_id = 0;
            FD_ZERO(&watching_connections);
            FD_SET(main_socket, &watching_connections);
            max_socket_id = std::max(max_socket_id,main_socket);
            FD_SET(data_socket, &watching_connections);
            max_socket_id = std::max(data_socket,main_socket);
            for (const Connection& connection : active_connections) {
                FD_SET(connection.child_socket_tcp, &watching_connections);
                max_socket_id = std::max(connection.child_socket_tcp,main_socket);
            }
            //select
            TIMEVAL timeout{0,timeout_ms}; //1 second timeout
            int active = select((int)max_socket_id + 1 , &watching_connections , nullptr , nullptr , &timeout);
            if(active < 0){throw std::runtime_error("Error selecting socket.");}
            if(active == 0) return; //Nothing more
            //Process each connection then remove it from the set
            for (int j = 0; j < active; ++j) {
                if(FD_ISSET(main_socket,&watching_connections)){  //New connection
                    //accept connection
                    Address address{};
                    int address_size = sizeof (address);
                    Socket new_socket = accept(main_socket,(struct sockaddr *)&address, (socklen_t*)&address_size);
                    if(new_socket == INVALID_SOCKET)  throw std::runtime_error("Error accepting socket.");
                    //add connection
                    active_connections.push_back(Connection{new_socket,address});
                    connection_callback((int)active_connections.size()-1,*this, false);
                    FD_CLR(main_socket,&watching_connections); //Remove from the set
                } else if(FD_ISSET(data_socket,&watching_connections)){  //Data packet
                    //todo udp and hashmap for client ID's
                } else{ //TCP packets
                    for (const Connection& connection : active_connections) {
                        if(FD_ISSET(connection.child_socket_tcp,&watching_connections)){ //TCP packet
                            //todo process packet data
                            //check for disconnect as well
                        }
                    }
                }
                //todo review error codes and types for POSIX vs WINSOCK
            }
        }
    }

    //todo client read,write,and connect
    //todo server server write

    /**
     * Close a connection
     * @param id Connection id. Must be valid.
     * Will not do anything if the manager is already closed or not a server
     */
    void closeConnection(int id){
        if(closed || !server) return;
        assert(id > -1 && id < active_connections.size());
        Connection connection = active_connections[id];
        active_connections.erase(active_connections.begin() + id);

        int status = shutdown(connection.child_socket_tcp, SD_BOTH);
        if (status == 0) { status = closesocket(main_socket); }
        if(status != 0) throw std::runtime_error("Error closing child socket.");
    }

    /**
     * Close the socket if not already closed
     * @throw runtime_error Error closing socket
     * Will also close child sockets.
     */
    void close(){
        if(closed) return;
        int status = 0;
        status = shutdown(main_socket, SD_BOTH);
        if (status == 0) { status = closesocket(main_socket); }
        if(status != 0) throw std::runtime_error("Error closing main socket.");
        status = shutdown(data_socket, SD_BOTH);
        if (status == 0) { status = closesocket(data_socket); }
        if(status != 0) throw std::runtime_error("Error closing data socket.");
        closed = true;
    }

    /**
     * Close the socket
     */
    ~ConnectionManager(){
        close();
    }


};

//todo add posix support
//todo multi threading