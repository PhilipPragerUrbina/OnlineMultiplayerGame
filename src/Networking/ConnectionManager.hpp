//
// Created by Philip on 7/29/2023.
//

#pragma once

#include <stdexcept>

#include <winsock2.h>
#include <Ws2tcpip.h>

/**
 * Use this class to connect to a server or to wait for and connect to multiple clients as a server
 * Allows a combination of TCP and UDP packets to be sent once connected
 * To actually send and receive packets between server and client, the connection class is used to manage that one single two-way connection
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
    Socket main_socket; //Used for creating connections
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
    }

    /**
     * Create a server socket bound to a port
     * @param port Port to bind to
     * @throw runtime_error Error starting socket
     * Uses any IP address it can.
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

    //todo see if child sockets are also closed when main socket closes and document
    /**
     * Close the socket if not already closed
     * @throw runtime_error Error closing socket
     * Make sure other connections are also closed.
     */
    void close(){
        if(closed) return;
        int status = 0;
        status = shutdown(main_socket, SD_BOTH);
        if (status == 0) { status = closesocket(main_socket); }
        if(status != 0) throw std::runtime_error("Error closing socket.");
        closed = true;
    }

    /**
     * Close the socket
     */
    ~ConnectionManager(){
        close();
    }

    //todo check for socket close
    //todo check if server

};

//todo add posix support