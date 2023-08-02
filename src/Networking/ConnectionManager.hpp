//
// Created by Philip on 7/29/2023.
//

#pragma once

#include <stdexcept>

#include <winsock2.h>
#include <Ws2tcpip.h>
#include <vector>
#include <unordered_map>
#include <cassert>

/**
 * This class can act as a client or a server for managing combined UDP and TCP connections.
 * @details Use this class to connect to a server as a client or to connect to multiple clients as a server(single threaded synchronous).
 * @warning Currently only works with WINSOCK. Todo: add POSIX support.
 * @note Multiple connection managers can be run at the same time.
 */
class ConnectionManager {
private:
    /**
     * Manage global WINSOCK initialization and clean up using static constructors and destructors
     * @details This can also just be done once per connection manager instance, as winsock has an internal counter.
     * Nevertheless, this ensures that winsock commands are valid for the entire lifetime of the program, just like POSIX.
     * @see https://learn.microsoft.com/en-us/windows/win32/api/winsock/nf-winsock-wsastartup
     */
    struct GlobalWinsock {
        /**
          * Start up WINSOCK version 2.2 globally.
          * @throw runtime_error Error starting winsock.
          */
        GlobalWinsock(){
            WSADATA winsock_data; //Not needed
            if(WSAStartup(MAKEWORD(2,2), &winsock_data) != 0 ) throw std::runtime_error("Error starting WINSOCK: " + std::to_string(WSAGetLastError()));
        }

        /**
          * Clean up winsock globally
          * @throw runtime_error Error closing winsock.
          */
        ~GlobalWinsock(){
            if(WSACleanup() != 0) throw std::runtime_error("Error closing WINSOCK: " + std::to_string(WSAGetLastError()));
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
    //Keep track of connections as a server
    std::unordered_map<u_long , Connection> active_connections; //long is ip address
    fd_set watching_connections;
    //Keep track of connection as a client
    Address server_address;
    //Shared sockets
    Socket main_socket; //Used for creating connections
    Socket data_socket; //UDP data socket

    bool server; //Is this a server or a client?

    /**
     * Maximum size of a single packet in bytes
     */
    const static int MAX_PACKET_SIZE = 256;
    char buffer[MAX_PACKET_SIZE];
private:

    /**
    * Receive a packet through UDP.
    * @param sender_address_out Outputs address of sender.
    * @return Vector containing data. Will return empty data if client disconnect or error.
    */
    std::vector<uint8_t> readUDP(Address& sender_address_out) {
        int address_size = sizeof(sender_address_out);
        int size  = recvfrom(data_socket, buffer, MAX_PACKET_SIZE, 0 , (SOCKADDR*)&sender_address_out, &address_size);
        if(size <= 0 ) return {};
        //copy data from buffer
        std::vector<uint8_t> raw_data(size);
        for (size_t i = 0; i < size; ++i) {
            raw_data[i] = buffer[i];
        }
        return raw_data;
    }

    /**
     * Receive a packet through TCP.
     * @param socket TCP socket.
     * @return Vector containing data. Will return empty data if an orderly client disconnect or error.
     */
    std::vector<uint8_t> readTCP(Socket socket) {
        int size  = recv(socket, buffer, MAX_PACKET_SIZE,0);
        if(size <= 0 ) return {};
        //copy data from buffer
        std::vector<uint8_t> raw_data(size);
        for (size_t i = 0; i < size; ++i) {
            raw_data[i] = buffer[i];
        }
        return raw_data;
    }

    /**
     * Write a packet through TCP.
     * @param socket TCP socket to write to.
     * @param data Data to write.
     * @return True, if sent successfully, false if an error (such as client disconnect).
     */
    static bool writeTCP(Socket socket, const std::vector<uint8_t>& data){
        int bytes_sent = send(socket, (const char *)(data.data()), (int)data.size(), 0);
        return bytes_sent == data.size();
    }

    /**
     * Write a packet through UDP
     * @param address Address to write to.
     * @param data Data to write.
     * @return True, if sent successfully, false if an error.
     */
    bool writeUDP(const Address& address, const std::vector<uint8_t>& data) const{
        int bytes_sent = sendto(data_socket, (const char *)(data.data()), (int)data.size(), 0,(const SOCKADDR * )(&address), sizeof(address));
        return bytes_sent == data.size();
    }

    /**
    * Do common initialization.
    * @throw runtime_error Error starting socket.
    */
    ConnectionManager() {
        main_socket = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
        if(main_socket == INVALID_SOCKET)  throw std::runtime_error("Error starting main socket: " + std::to_string(WSAGetLastError()));
        data_socket = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
        if(data_socket == INVALID_SOCKET)  throw std::runtime_error("Error starting data socket: " + std::to_string(WSAGetLastError()));
    }

    /**
    * Connect to a server as a client
    * @warning Must be a client and not closed.
    * @param address Server address.
    * @throw runtime_error Error connecting.
    */
    void connectToServer(const Address& address){
        assert(!server);
        server_address = address;
        if(connect(main_socket, (SOCKADDR*)&address,sizeof(address)) != 0 ){throw std::runtime_error("Error connecting to server: " + std::to_string(WSAGetLastError()));};
    }

public:

    /**
     * Create a client socket that can connects to a server.
     * Is not bound to a particular port.
     * @param server_address Address of server to connect to.
     * @throw runtime_error Error starting socket or connecting.
     */
    explicit ConnectionManager(const Address& server_address) : ConnectionManager() {
        server = false;
        main_socket = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
        if(main_socket == INVALID_SOCKET)  throw std::runtime_error("Error starting main socket: " + std::to_string(WSAGetLastError()));
        data_socket = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
        if(data_socket == INVALID_SOCKET)  throw std::runtime_error("Error starting data socket: " + std::to_string(WSAGetLastError()));
        connectToServer(server_address);
    }

    /**
     * Create a server socket bound to a port.
     * @param port Port to bind to.
     * @throw runtime_error Error starting socket.
     * Uses any IP address it can.
     * It will start listening right away.
     */
    explicit ConnectionManager(Port port) : ConnectionManager() { //Create socket
        server = true;
        //Set socket mode
        int opt = 1;
        if(setsockopt(main_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt,sizeof(opt)) != 0) throw std::runtime_error("Error setting socket options: " + std::to_string(WSAGetLastError()));
        //Bind sockets
        sockaddr_in server_options{};
        server_options.sin_port = htons(port);
        server_options.sin_family = AF_INET;
        server_options.sin_addr.s_addr = htonl(INADDR_ANY);
        if(bind(main_socket, (SOCKADDR *) &server_options, sizeof(server_options)) != 0) throw std::runtime_error("Error binding main socket: " + std::to_string(WSAGetLastError()));
        if(bind(data_socket, (SOCKADDR *) &server_options, sizeof(server_options)) != 0) throw std::runtime_error("Error binding data socket: " + std::to_string(WSAGetLastError()));
        const int CONNECTION_QUEUE_SIZE = 10;
        if(listen(main_socket,CONNECTION_QUEUE_SIZE) != 0 ) { throw std::runtime_error("Error listening: " + std::to_string(WSAGetLastError())); };
    }

    /**
      * Get the address struct from address information
      * @param port Address port.
      * @param ip_address IPV4 IP address in string format.
      * @return Address struct.
      * @warning IP must be in valid format. See: https://learn.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-inet_addr
      */
    static Address getAddress(Port port,const std::string& ip_address){
        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_port = htons(port);
        address.sin_addr.s_addr = inet_addr(ip_address.c_str());
        return address;
    }

    /**
     * Process all currently incoming packets.
     * Will accept new connections, disconnects, and will process UDP and TCP messages.
     * @warning  Must be a server!
     * @param receive_callback Callback for incoming UDP and TCP packets.
     * @details TCP boolean is true if TCP false is UDP. Client ID is a unique long IP address. The packet data is a copy of the data sent.
     * The manager is passed by reference such that a response can be sent right away if needed using the client id.
     * @param connection_callback Callback for new connection or disconnect.
     * @param timeout_ms Time period to wait for packets in milliseconds. Useful if application has a tick rate.
     * @param max_packets The maximum number of simultaneous or consecutive UDP packets that would be expected to arrive within the timeout. Just used as an upper bound for how many times to select.
     * @throw runtime_error Error regarding socket selection or connection. If a problem is encountered with a specific client, no error will be thrown, the client will just be considered disconnected. (Handle using disconnect callback).
     */
    void processIncoming(void (*receive_callback)(bool TCP, u_long client_id, const std::vector<uint8_t>& packet_data,ConnectionManager& manager),
                         void (*connection_callback)(u_long client_id,ConnectionManager& manager,bool disconnect),
                         int timeout_ms = 50, int max_packets = 20){
        assert(server);
        //Collect multiple messages from one socket if needed
        for (int i = 0; i < max_packets; ++i) {
            //Add connections
            Socket max_socket_id = 0;
            FD_ZERO(&watching_connections);
            FD_SET(main_socket, &watching_connections);
            max_socket_id = std::max(max_socket_id,main_socket);
            FD_SET(data_socket, &watching_connections);
            max_socket_id = std::max(max_socket_id,data_socket);
            for (const auto & [ id, connection ] : active_connections) {
                FD_SET(connection.child_socket_tcp, &watching_connections);
                max_socket_id = std::max(max_socket_id,connection.child_socket_tcp);
            }

            //select
            TIMEVAL timeout{0,timeout_ms};
            int active = select((int)max_socket_id + 1 , &watching_connections , nullptr , nullptr , &timeout);
            if(active < 0){throw std::runtime_error("Error selecting socket: " + std::to_string(WSAGetLastError()));}
            if(active == 0) return; //Nothing more

            //Process each connection then remove it from the set
            for (int j = 0; j < active; ++j) {
                if(FD_ISSET(main_socket,&watching_connections)){  //New connection
                    FD_CLR(main_socket,&watching_connections); //Remove from the set

                    //accept connection
                    Address address{};
                    int address_size = sizeof (address);
                    Socket new_socket = accept(main_socket,(SOCKADDR*)&address, (socklen_t*)&address_size);
                    if(new_socket == INVALID_SOCKET)  throw std::runtime_error("Error accepting socket: " + std::to_string(WSAGetLastError()));

                    //add connection
                    active_connections[address.sin_addr.s_addr] = Connection{new_socket,address};
                    connection_callback(address.sin_addr.s_addr,*this, false);
                } else if(FD_ISSET(data_socket,&watching_connections)){  //Incoming data packet
                    FD_CLR(data_socket,&watching_connections); //Remove from the set

                    Address udp_address;
                    std::vector<uint8_t> data = readUDP(udp_address);
                    if(!data.empty() && active_connections.find(udp_address.sin_addr.s_addr) != active_connections.end()){ //Must be from existing connection
                        receive_callback(false,udp_address.sin_addr.s_addr,data,*this);
                    }
                } else{ //Incoming TCP packets
                    std::vector<u_long> sockets_to_close; //defer closing until after iteration
                    for (const auto & [ id, connection ] : active_connections) {
                        if(FD_ISSET(connection.child_socket_tcp,&watching_connections)){ //Incoming TCP packet
                            FD_CLR(connection.child_socket_tcp,&watching_connections); //Remove from the set
                            std::vector<uint8_t> data = readTCP(connection.child_socket_tcp);
                            if(data.empty()){ //disconnect
                                connection_callback(id,*this,true);
                                sockets_to_close.push_back(id);
                            }else{
                                receive_callback(true,id,data,*this);
                            }
                        }
                    }
                    for (const u_long& socket_to_close : sockets_to_close) {
                        closeConnection(socket_to_close);
                    }
                }
            }

        }
    }

    /**
     * Process all currently incoming packets as a client.
     * Will process UDP and TCP messages.
     * @warning  Must be a client!
     * @param receive_callback Callback for incoming UDP and TCP packets.
     * @details TCP boolean is true if TCP false is UDP. The packet data is a copy of the data sent.
     * The manager is passed by reference such that a response can be sent right away.
     * @param timeout_ms Time period to wait for packets in milliseconds. Useful if application has a tick rate.
     * @param max_packets The maximum number of simultaneous or consecutive UDP packets that would be expected to arrive within the timeout. Just used as an upper bound for how many times to select.
     * @throw runtime_error Error regarding socket selection or connection.
     * @return False if server is disconnected.
     */
    bool processIncoming(void (*receive_callback)(bool TCP, const std::vector<uint8_t>& packet_data,ConnectionManager& manager),int timeout_ms = 50, int max_packets = 20){
        assert(!server);
        //Collect multiple messages from one socket if needed
        for (int i = 0; i < max_packets; ++i) {

            //Add connections
            Socket max_socket_id = 0;
            FD_ZERO(&watching_connections);
            FD_SET(main_socket, &watching_connections);
            max_socket_id = std::max(max_socket_id,main_socket);
            FD_SET(data_socket, &watching_connections);
            max_socket_id = std::max(max_socket_id,data_socket);

            //select
            TIMEVAL timeout{0,timeout_ms};
            int active = select((int)max_socket_id + 1 , &watching_connections , nullptr , nullptr , &timeout);
            if(active < 0){throw std::runtime_error("Error selecting socket: " + std::to_string(WSAGetLastError()));}
            if(active == 0) return true; //Nothing more

            //Process each connection then remove it from the set
            for (int j = 0; j < active; ++j) {
               if(FD_ISSET(data_socket,&watching_connections)){  //Incoming data packet

                    Address udp_address;
                    std::vector<uint8_t> data = readUDP(udp_address);

                    if(!data.empty()){
                        receive_callback(false,data,*this);
                    }
                    FD_CLR(data_socket,&watching_connections); //Remove from the set
                } else if(FD_ISSET(main_socket,&watching_connections)){ //Incoming TCP packet
                    std::vector<uint8_t> data = readTCP(main_socket);

                    if(data.empty()){ //disconnect
                        return false;
                    }else{
                        receive_callback(true,data,*this);
                    }

                    FD_CLR(main_socket,&watching_connections); //Remove from the set
                }
            }
        }
        return true;
    }

    /**
     * Write a packet to a client through UDP as a server.
     * @warning Must be a server.
     * @param client_id Valid client ID to write to.
     * @param data Data to write.
     */
    void writeUDP(u_long client_id, const std::vector<uint8_t>& data) {
        assert(server);
        writeUDP(active_connections[client_id].other_address,data);
    }

    /**
     * Write a packet to a client through TCP as a server.
     * @warning Must be a server.
     * @param client_id Valid client ID to write to.
     * @param data Data to write.
     * @return True if sent successfully. False probably indicates a client disconnect or network issue.
     */
    bool writeTCP(u_long client_id, const std::vector<uint8_t>& data) {
        assert(server);
        return writeTCP(active_connections[client_id].child_socket_tcp,data);
    }

    /**
     * Write a packet to the connected server through TCP as a client.
     * @warning Must be a client.
     * @param data Data to write.
     * @return True if sent successfully. False probably indicates a server disconnect or network issue.
     */
    bool writeTCP(const std::vector<uint8_t>& data) const{
        assert(!server);
        return writeTCP(main_socket,data);
    }

    /**
     * Write a packet to the connected server through UDP as a client.
     * @warning Must be a client.
     * @param data Data to write.
     */
    void writeUDP(const std::vector<uint8_t>& data){
        assert(!server);
        writeUDP(server_address,data);
    }

    /**
     * Close a specific connection.
     * @param client_id Client id. Must be valid, otherwise undefined behavior.
     * @throw runtime_error Error closing socket.
     * @warning Must be a server!
     */
    void closeConnection(u_long client_id){
        assert(server);

        Connection connection = active_connections[client_id];
        active_connections.erase(client_id);
        shutdown(connection.child_socket_tcp, SD_BOTH);
        if(closesocket(connection.child_socket_tcp) != 0) throw std::runtime_error("Error closing child socket: " + std::to_string(WSAGetLastError()));
    }

    /**
     * Close the socket.
     * @throw runtime_error Error closing sockets.
     * Will also close child sockets.
     */
    ~ConnectionManager(){
        shutdown(main_socket, SD_BOTH);
        if(closesocket(main_socket) != 0) throw std::runtime_error("Error closing main socket: " + std::to_string(WSAGetLastError()));
        shutdown(data_socket, SD_BOTH);
        if(closesocket(data_socket) != 0) throw std::runtime_error("Error closing data socket: " + std::to_string(WSAGetLastError()));
    }
};