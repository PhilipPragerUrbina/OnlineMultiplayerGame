//
// Created by Philip on 7/28/2023.
//

#pragma once

#include <stdexcept>

#ifdef _WIN32
#include <winsock2.h>
#include <Ws2tcpip.h>
#else
#include <sys/socket.h>
  #include <arpa/inet.h>
  #include <netdb.h>
  #include <unistd.h>
#endif

/**
 * Max size a packet can be
 */
const int MAX_PACKET_SIZE_TCP = 2048;

/**
 * Represents a TCP socket that can send and receive
 * Is a cross-platform wrapper for winsock and POSIX
 */
class TCPSocket {
public:

#ifdef _WIN32
    typedef SOCKET Socket;
#else
    typedef int Socket;
#endif
    typedef unsigned short Port;
    typedef sockaddr_in  Address;
private:
    Socket current_socket;
    bool closed = false;
    char buffer[MAX_PACKET_SIZE_TCP];
public:

    /**
     * Create a client socket not bound to a port
     *  @throw runtime_error Error starting socket
     */
    TCPSocket(){
    #ifdef _WIN32
            WSADATA wsa_data;
            if(WSAStartup(MAKEWORD(2,2), &wsa_data)!=0) throw std::runtime_error("Error starting winsock.");
    #endif

    #ifdef _WIN32
            current_socket = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
            if(current_socket == INVALID_SOCKET)  throw std::runtime_error("Error starting socket.");
    #else
            current_socket = socket(AF_INET,SOCK_STREAM,0);
            if(current_socket < 0>)  throw std::runtime_error("Error starting socket.");
    #endif
    }

    /**
     * Create a server socket bound to a port
     * @param port Port to bind to
     * @throw runtime_error Error starting socket
     */
    TCPSocket(Port port) : TCPSocket(){ //Start socket
        int opt = 1;
        if( setsockopt(current_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt,sizeof(opt)) < 0 ) throw std::runtime_error("Error setting socket.");

        //Bind socket(Same for both platforms)
        sockaddr_in server_options{};
        server_options.sin_port = htons(port);
        server_options.sin_family = AF_INET;
        server_options.sin_addr.s_addr = htonl(INADDR_ANY);
        if(bind(current_socket, (SOCKADDR *) &server_options, sizeof(server_options)) != 0) throw std::runtime_error("Error binding socket.");
    }

    /**
     * Send data through this socket.
     * @param raw_data Pointer to data to send. Ownership is not taken.
     * @param size Size of data in bytes.
     * @throw runtime_error Error sending data
     * Will not send if closed.
     */
    void sendTo(const char* raw_data, size_t size) const{
        if(closed) return;
        if(send(current_socket,  raw_data, (int)size, 0) < 0) throw std::runtime_error("Error sending.");
    }

    /**
     * Read data
     * @param size_out Outputs size of data in bytes
     *  Will not receive if closed, will return nullptr.
     *  @throw runtime_error Error receiving data
     *  @return Pointer to new data. Ownership is then yours.
     */
    char* receiveFrom(size_t& size_out) {
        if(closed) return nullptr;

        int out  = recv(current_socket, buffer, MAX_PACKET_SIZE_TCP, 0);
        if(out < 0 ) throw std::runtime_error("Error receiving.") ;
        size_out = out;

        //copy data from buffer
        char* raw_data = new char[size_out];
        for (size_t i = 0; i < size_out; ++i) {
            raw_data[i] = buffer[i];
        }
        return raw_data;
    }

    /**
     * Listen for an incoming connection
     * @throw runtime_error Error listening
     */
    void listenFor() const {
        if(listen(current_socket,3) < 0) throw std::runtime_error("Error listening.");
    }

    /**
     * Connect to a server
     * @param address Address of server
     *  @throw runtime_error Error connecting
     */
    void connectTo(const Address& address) const {
        if(connect(current_socket,(SOCKADDR *)&address, sizeof(address)) < 0) throw std::runtime_error("Error connecting.");
    }


    /**
     * Get the address struct from address information
     * @param port Address port
     * @param ip_address IPV4 ip address in string format
     * @return Address struct
     */
    static Address getAddress(Port port,const std::string& ip_address){
        sockaddr_in address;
        address.sin_family = AF_INET;
        address.sin_port = htons(port);
        address.sin_addr.s_addr = inet_addr(ip_address.c_str());
        return address;
    }

    /**
     * Close the socket
     * @throw runtime_error Error closing socket
     * Will not close if already closed.
     */
    void close(){
        if(closed) return;
        int status = 0;
#ifdef _WIN32
        status = shutdown(current_socket, SD_BOTH);
        if (status == 0) { status = closesocket(current_socket); }
#else
        status = shutdown(sock, SHUT_RDWR);
        if (status == 0) { status = close(sock); }
#endif
        if(status != 0) throw std::runtime_error("Error closing socket.");
        closed = true;
    }

    /**
     * Clean up and close socket if needed
     * @throw runtime_error Error closing socket
     */
    ~TCPSocket(){
        if(!closed) close();
#ifdef _WIN32
        if(WSACleanup() != 0) throw std::runtime_error("Error closing winsock.");
#endif
    }
};
