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
#include "GameState/AIPlayer.hpp"
#include "GameState/Car.hpp"

/**
 * Contains information about a client
 */
struct ClientInfo{
    std::unordered_set<uint16_t> associated_objects{}; //Objects assigned to this client.
    // Always visible and are relayed events from a client.
    uint8_t current_event_counter = 0; //Counter of current event to make sure new events are more recent
    std::unordered_set<uint16_t> cached_objects; //Objects the client has been told to instantiate.
    bool handshake = false; //If the client has initiated a handshake
    Camera camera{90,{0,0,1},1};
};

/**
 * Keeps game state and simulates a game world.
 * Sends relevant game state to a client and receive input events from the clients (Server authoritative).
 */
class Server {
private:

    Services services{}; //Services used by gameobjects. (Write Update thread)
    ResourceManager resource_manager{}; //Shared between game objects read only resources (Write Update)
    std::unordered_map<ObjectID,ClientEvents> object_events{}; //Any objects with events assigned to them. (Write Update thread)

    std::unordered_map<u_long,ClientInfo> clients{}; //Clients currently connected. (Write Network thread)
    ConnectionManager network; //Connection to clients. (Write Network thread)

    moodycamel::ReaderWriterQueue<std::pair<ObjectID ,ClientEvents>> incoming_events{}; //New events to add to objects (Network thread -> Update thread)
    moodycamel::ReaderWriterQueue<std::pair<ObjectID ,std::unique_ptr<GameObject>>> new_object_queue; //New objects created by network thread. (Network thread -> Update thread)
    moodycamel::ReaderWriterQueue<ObjectID> remove_object_queue; //Objects the network thread wants to destroy. (Network thread -> Update thread)
    //todo allow new objects to be spawned by game objects or services.

    std::thread network_thead,update_thread;
    std::atomic<bool> running = true; //(Read Network thread & Write Update thread)
    std::atomic<ObjectID> latest_object_id = 0; //Latest available object id. (Write Network thread & Write thread)

    std::unique_ptr<std::unordered_map<ObjectID,std::unique_ptr<GameObject>>> objects_buffer_network; //Game object buffer for network thread. (Read Network thread & Write Update thread)
    std::unique_ptr<std::unordered_map<ObjectID,std::unique_ptr<GameObject>>> objects_buffer_update; //Game object buffer to be written to by update thread.(Write Update thread)
    std::mutex swap_mutex;

    /**
     * Swap network and update buffer pointers.
     * Also copies data such that they are equal.
     * @warning Only called by update thread
     */
    void swapBuffers(){
        //Make sure swap is safe
        {
            std::lock_guard guard(swap_mutex);
            std::swap(objects_buffer_update,objects_buffer_network);
        }
        //Copy data from current network buffer to update buffer to apply previous changes.
        //This is fine without a mutex, since it is just a multiple read of the network buffer. And only the update thread uses the update buffer, so it can write to it.
        objects_buffer_update->clear();
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
                if(hand_shake.version != PROTOCOL_VERSION){
                    //todo disconnect client and make sure to do the same things as in the connectCallback. Also make sure it wont mess anything up in the connection manager and document.
                    std::cerr << "Warning: Client " << client_id << " has mismatched version \n";
                }else{
                    clients[client_id].handshake = true;
                }
                std::cout << "Client " << client_id << " has handshake \n";
            } else if(type.type == CAMERA_CHANGE){
                auto new_settings = extractStructFromPacket<CameraChange>(packet_data,sizeof(MessageTypeMetaData));
                clients[client_id].camera = Camera{glm::degrees(new_settings.fov_radians),{0,0,-1},new_settings.aspect_ratio}; //todo standardized directions header
            }

        }else{  //data packet must be a client event
            auto client_message = extractStructFromPacket<ClientEvents>(packet_data,0);
            ClientInfo& client = clients[client_id];

            if(client_message.counter >= client.current_event_counter || client.current_event_counter == 255){ //If more recent. Taking into account wrapping.
                client.current_event_counter = client_message.counter;
                for (const ObjectID & object_id : client.associated_objects) {
                    incoming_events.emplace(object_id,client_message);
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
            clients[client_id] = ClientInfo{{},0,{}};
            //todo init associated objects here.
           // clients[client_id].associated_objects.emplace(addGameObjectNetworkThread(new Player()));
            clients[client_id].associated_objects.emplace(addGameObjectNetworkThread(new Car()));
        }
    }


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
                },TICK_RATE,50);

            //Send messages
            for (auto& [client_id, client] : clients) {
                if(!client.handshake) continue;

                std::lock_guard guard (swap_mutex); //Mutex needed to make sure swap does not happen during reading.

                //update client cameras
                glm::vec3 new_position = {2,2,2}; //differ default values to avoid nans
                glm::vec3 new_look_at = {0,0,0};
                for (const ObjectID& object_id : client.associated_objects) {
                    if((*objects_buffer_network)[object_id]->updateCamera(new_position,new_look_at)){
                        client.camera.setPosition(new_position);
                        client.camera.setPosition(new_look_at);
                        break;
                    }
                }

                uint8_t buffer_location = 0;
                for (const auto & [object_id, game_object] : *objects_buffer_network) {

                    //not associated and not visible
                    if(!game_object->getBounds().inFrustum(client.camera) && client.associated_objects.find(object_id) == client.associated_objects.end()) continue;

                    std::vector<uint8_t> data;
                    if(client.cached_objects.find(object_id) == client.cached_objects.end()){
                        //must create a new object
                        MessageTypeMetaData type{NEW_OBJECT};
                        addStructToPacket(data,type);
                        NewObjectMetaData new_obj{game_object->getTypeID() ,object_id, client.associated_objects.find(object_id) != client.associated_objects.end()};
                        addStructToPacket(data,new_obj);
                        game_object->getConstructorParams(data);
                        if(network.writeTCP(client_id,data)){
                            client.cached_objects.emplace(object_id);
                            break; //Only one object created at a time for timing reasons.
                        }
                    }else{
                        if(buffer_location > MAX_VISIBLE_OBJECTS){
                            continue; //Client ran out of space in visibility buffer.
                        }
                        //must update object
                        StateMetaData meta_data{buffer_location,object_id};
                        addStructToPacket(data,meta_data);
                        game_object->serialize(data);
                        network.writeUDP(client_id,data);
                        buffer_location++;
                    }
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds (TICK_RATE)); //I know this doesn't actually enforce tick rate, but it doesn't really matter.
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
        //Make AI player
       // (*objects_buffer_update)[latest_object_id] = std::unique_ptr<GameObject>(new AIPlayer());
       // latest_object_id++;

        //load resources and register services
        for (const auto& [id, object_ptr] : *objects_buffer_update) {
            object_ptr->loadResourcesServer(resource_manager);
            object_ptr->registerServices(services);
        }

        auto last_update = std::chrono::steady_clock::now();
        while(running){
            //Get delta time.
            auto now = std::chrono::steady_clock::now();
            auto delta_time = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_update).count();
            last_update = now;

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
            std::pair<ObjectID, ClientEvents> new_event;
            while (incoming_events.try_dequeue(new_event)) {
                object_events[new_event.first] = new_event.second;
            }

            //Update services all at once to minimize the effect of object ordering.
            for (const auto & [id, game_object] : *objects_buffer_update) {
                game_object->updateServices(services);
            }

            //Update objects. No mutex needed as swap happens on this thread.
            for (const auto & [id, game_object] : *objects_buffer_update) {
                EventList event_list{}; //start with empty event
                int object_delta_time = delta_time;
                if (object_events.find(id) != object_events.end()) {
                    event_list = object_events[id].list; //get associated events
                    object_delta_time = object_events[id].milliseconds; //Use client frame time
                }
                game_object->update(object_delta_time,event_list,services,resource_manager);
            }
            //swap buffers
            swapBuffers();
        }
        //No need to deregister services, as the server is ending anyway.
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
