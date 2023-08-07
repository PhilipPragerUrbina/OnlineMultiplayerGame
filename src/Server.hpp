//
// Created by Philip on 8/4/2023.
//

#pragma once

#include <vector>
#include <atomic>
#include <chrono>
#include <thread>
#include <mutex>
#include <unordered_set>
#include "GameState/GameObject.hpp"
#include "Services/Services.hpp"
#include "readwriterqueue/readerwriterqueue.h"
#include "Networking/ConnectionManager.hpp"
#include "GameState/GameMap.hpp"
#include "GameState/Player.hpp"

/**
 * Contains information about a client
 */
struct ClientInfo{
    u_long id{}; //IP address
    std::unordered_set<uint16_t> associated_objects{}; //Objects assigned to this client. Always visible and are relayed events from a client.
    uint8_t current_event_counter = 0; //Counter of current event to make sure new events are more recent
    std::unordered_set<uint16_t> cached_objects; //Objects the client has been told to instantiate.
};
/**
 * Keeps game state and simulates a game world.
 * Sends relevant game state to a client and receive input events from the clients (Server authoritative).
 */
class Server {
private:
    //Unique id for a game object
    typedef uint16_t ObjectID; //todo swap to this typedef globally for clarity

    Services services{}; //Services used by gameobjects. (Update thread)
    std::unordered_map<ObjectID,EventList> object_events{}; //Any objects with events assigned to them (Update thread)

    std::unordered_map<u_long,ClientInfo> clients{}; //Clients currently connected (Network thread)
    ConnectionManager network; //Connection (Network thread)

    moodycamel::ReaderWriterQueue<std::pair<ObjectID ,EventList>> incoming_events{}; //New events to add to objects (Network thread -> Update thread)
    moodycamel::ReaderWriterQueue<std::pair<ObjectID ,std::unique_ptr<GameObject>>> new_object_queue; //New objects created by network thread. (Network thread -> Update thread)
    //todo allow new objects to be created by game objects.
    moodycamel::ReaderWriterQueue<ObjectID> remove_object_queue; //Objects the network thread wants to destroy (Network thread -> Update thread)

    std::thread network_thead,update_thread;
    std::atomic<bool> running = true; //(Network thread & Update thread)
    std::atomic<ObjectID> latest_object_id = 0; //Latest available object id. (Network thread & Update thread)
    ResourceManager resource_manager{}; //Shared between game objects read only resources (Network)

    static const int VERSION = 0; //Version to check for compatibility (Network thread)

    std::unique_ptr<std::unordered_map<ObjectID,std::unique_ptr<GameObject>>> objects_buffer_network; //Game object buffer for both threads to READ (Network thread & Update thread)
    std::unique_ptr<std::unordered_map<ObjectID,std::unique_ptr<GameObject>>> objects_buffer_update; //Game object buffer to be written to by update thread(Update thread)
    std::mutex swap_mutex;

    /**
     * Swap network and update buffer pointers.
     * Also copies data such that they are equal.
     * Happens on update thread.
     */
    void swapBuffers(){
        //Make sure swap is safe
        {
            std::lock_guard guard(swap_mutex);
            std::swap(objects_buffer_update,objects_buffer_network);
        }
        //Copy data from current network buffer to update buffer to apply previous changes.
        //This is fine without a mutex, since it is just a multiple read of the network buffer. And only the update thread uses the update buffer, so it can write to it.
        objects_buffer_update->clear(); //todo optimize the clear deleting all the objects that could still be used. But objects that are not present in the other buffer should be deleted.
        objects_buffer_update->reserve(objects_buffer_network->size());
        for (const auto& [id, object_ptr] : *objects_buffer_network) {
            (*objects_buffer_update)[id] = object_ptr->copy();
        }
    }

    /**
     * Manage incoming client messages
     */
    void receiveCallback(bool TCP, u_long client_id, const std::vector<uint8_t>& packet_data,ConnectionManager& manager){
        if(TCP){
            auto type = extractStructFromPacket<MessageTypeMetaData>(packet_data,0);

            if(type.type == HANDSHAKE){
                auto hand_shake = extractStructFromPacket<HandShake>(packet_data,sizeof(MessageTypeMetaData));
                if(hand_shake.version != VERSION){
                    //todo disconnect client and make sure to do the same things as in the connectCallback. Also make sure it wont mess anything up in the connection manager and document.
                    std::cerr << "Warning: Client " << client_id << " has mismatched version \n";
                }
                std::cout << "Client " << client_id << " has handshake \n";
            }

        }else{  //data packet must be a client event
            auto client_message = extractStructFromPacket<ClientEvents>(packet_data,0);
            ClientInfo& client = clients[client_id];

            if(client_message.counter > client.current_event_counter || client_message.counter == 0){ //If more recent. Taking into account wrapping.
                client.current_event_counter = client_message.counter;
                for (const ObjectID & object_id : client.associated_objects) {
                    incoming_events.emplace(object_id,client_message.list);
                }
            }
        }
    }

    /**
     * Create a game object on the network thread and queue it to be added to the update thread.
     * @param object Pointer to a new game object. Ownership will be taken.
     * @return Object id of the new object.
     */
    ObjectID addGameObjectNetworkThread(GameObject* object){
        new_object_queue.emplace(std::pair<ObjectID,std::unique_ptr<GameObject>>( latest_object_id ,std::unique_ptr<GameObject>(object)));
        latest_object_id++;
        return latest_object_id - 1;
    }

    /**
     * Detect when clients join or disconnect and create their associated objects
     */
    void connectionCallback(u_long client_id,ConnectionManager& manager,bool disconnect){
        if(disconnect){
            std::cout << "Client " << client_id << " has disconnected \n";
            //Destroy associated objects
            for (const ObjectID& object_id : clients[client_id].associated_objects) {
                remove_object_queue.enqueue(object_id);
            }
            clients.erase(client_id);
        }else{
            std::cout << "Client " << client_id << " has connected \n";
            clients[client_id] = ClientInfo{client_id,{},0,{}};
            //todo init associated objects here.
            clients[client_id].associated_objects.emplace(addGameObjectNetworkThread(new Player()));
        }
    }

    uint8_t network_counter = 0; //Used for UDP packet ordering
    /**
     * Wait for incoming events and send out game state
     */
    void networkThread(){
        while(running){
            //Gather messages
            network.processIncoming([this](bool TCP, u_long client_id, const std::vector<uint8_t>& packet_data,ConnectionManager& manager){
                        this->receiveCallback(TCP,client_id,packet_data,manager);
                },[this](u_long client_id,ConnectionManager& manager,bool disconnect){
                    this->connectionCallback(client_id,manager,disconnect);
                },tick_rate,50); //todo check tick

            //Send messages
            for (auto& [client_id, client] : clients) {
                //todo get visible list based on frustum culling + associated objects
                //todo max buffer size
                uint8_t buffer_location = 0;
                std::lock_guard guard (swap_mutex); //Mutex needed to make sure swap does not happen during reading.
                for (const auto & [object_id, game_object] : *objects_buffer_network) {
                    std::vector<uint8_t> data;
                    if(client.cached_objects.find(object_id) == client.cached_objects.end()){
                        //must create a new object
                        MessageTypeMetaData type{NEW_OBJECT};
                        addStructToPacket(data,type);
                        NewObjectMetaData new_obj{game_object->getTypeID() ,object_id, client.associated_objects.find(object_id) != client.associated_objects.end()};
                        addStructToPacket(data,new_obj);
                        game_object->getConstructorParams(data);
                        network.writeTCP(client_id,data);
                        client.cached_objects.emplace(object_id);
                    }else{
                        //must update object
                        StateMetaData meta_data{buffer_location,object_id,network_counter};
                        addStructToPacket(data,meta_data);
                        game_object->serialize(data);
                        network.writeUDP(client_id,data);
                        network_counter = (network_counter + 1) % 256; //explicit wrap
                        buffer_location++;
                    }
                }
            }
        }
    }

    /**
     * Update the game state
     */
    void updateThread(){
        //todo add game objects here
        //make map
        (*objects_buffer_update)[latest_object_id] = std::unique_ptr<GameObject>(new GameMap());
        latest_object_id++;

        //load resources and register services
        for (const auto& [id, object_ptr] : *objects_buffer_update) {
            object_ptr->loadResourcesServer(resource_manager);
            object_ptr->registerServices(services);
        }

        auto last_update = std::chrono::steady_clock::now();
        while(running){

            //destroy objects as needed
            uint16_t remove_obj_id;
            while (remove_object_queue.try_dequeue(remove_obj_id)) {
                (*objects_buffer_update)[remove_obj_id]->deRegisterServices(services); //Make sure to remove from services
                objects_buffer_update->erase(remove_obj_id);
            }
            //create objects as needed
            std::pair<ObjectID, std::unique_ptr<GameObject>> new_obj;
            while (new_object_queue.try_dequeue(new_obj)) {
                (*objects_buffer_update)[new_obj.first] = std::move(new_obj.second);
                (*objects_buffer_update)[new_obj.first]->loadResourcesServer(resource_manager);
                (*objects_buffer_update)[new_obj.first]->registerServices(services);
            }
            //Load events
            std::pair<ObjectID, EventList> new_event;
            while (incoming_events.try_dequeue(new_event)) {
                object_events[new_event.first] = new_event.second;
            }

            //Update services all at once to minimize the effect of object ordering.
            for (const auto & [id, game_object] : *objects_buffer_update) {
                game_object->updateServices(services);
            }

            auto now = std::chrono::steady_clock::now();
            double delta_time = (double)std::chrono::duration_cast<std::chrono::microseconds>(now - last_update).count() / 1000000.0f;
            last_update = now;
            //Update objects. No mutex needed as swap happens on this thread.
            for (const auto & [id, game_object] : *objects_buffer_update) {
                EventList event_list{}; //start with empty event
                if (object_events.find(id) != object_events.end()) {
                    event_list = object_events[id]; //get associated events
                }
                game_object->update(delta_time,event_list,services,resource_manager);
            }
            //swap buffers
            swapBuffers();
        }

        //todo deregister services. Not really needed since services will be deleted anyway.
    }

public:
    /**
     * Create a new game server on a port
     * @param port Port to start server on.
     * Will start running on multiple threads right away.
     */
    explicit Server(ConnectionManager::Port port) : network(port) {
        objects_buffer_update = std::make_unique<std::unordered_map<ObjectID,std::unique_ptr<GameObject>>>();
        objects_buffer_network = std::make_unique<std::unordered_map<ObjectID,std::unique_ptr<GameObject>>>();
        network_thead = std::thread(&Server::networkThread, this);
        update_thread = std::thread(&Server::updateThread, this);
    }

    /**
     * Stop the server if running.
     */
    void stop(){
        if(running){
            running = false;
            update_thread.join();
            network_thead.join();
        }
    }

    /**
     * Stops server,
     */
    ~Server(){
        stop();
    }
};
