//
// Created by Philip on 8/4/2023.
//

#pragma once

#include <atomic>
#include <memory>
#include <thread>
#include <mutex>
#include "Renderer/SDL/Window.hpp"
#include "Renderer/Renderer.hpp"
#include "GameState/GameObject.hpp"
#include "readwriterqueue/readerwriterqueue.h"
#include "Networking/ConnectionManager.hpp"

/**
 * Connects to server and runs game loop
 */
class Client {
public:
    //settings
    static const int WIDTH = 800;
    static const int HEIGHT = 800;
    static const int MAX_VISIBLE_OBJECTS = 15;
private:

    Window window{WIDTH,HEIGHT,"Client"}; // Poll events and display the frame buffer.(Write Rendering thread & Read Update thread)
    std::atomic<bool> running = true;  //Tells threads when to stop.(Read Rendering thread & Read Network thread
    // & Write Update thread)

    ResourceManager resource_manager{}; //Manage read only shared resources.(Read Rendering thread & Mutex Write Update thread)
    std::mutex resource_mutex; //Sometimes update thread needs to add new resources

    //Visible object double buffers.(Update thread -> Render thread)
    std::mutex visibility_buffer_mutex;
    std::array<std::unique_ptr<GameObject>,MAX_VISIBLE_OBJECTS> render_buffer{nullptr}; //Contains copies of objects for rendering.(Write Render thread)
    std::array<GameObject*,MAX_VISIBLE_OBJECTS> update_buffer{nullptr}; //Contains pointers to object cache for updating objects.(Write Update thread & Read mutex Render thread)

    FrameBuffer frame_buffer{WIDTH,HEIGHT,{0,0,0,0}}; //Main frame buffer.(Write Rendering thread)
    Renderer renderer{Camera{90,{0,0,1}, (float)WIDTH / HEIGHT},WIDTH,HEIGHT}; //Main rendering engine.(Write Rendering thread)

    std::unordered_map<uint16_t , std::unique_ptr<GameObject>> object_cache{}; //Contains instantiated objects.(Write Update thread)
    Services services{};  //Allows game objects to communicate.(Write Update thread)

    ConnectionManager network; //Connects with server.(Write Network thread)
    uint8_t network_counter = 0; //Used for UDP packet ordering.(Write Network thread)

    moodycamel::ReaderWriterQueue<ConnectionManager::RawData> incoming_objects{}; //New objects to instantiate.(Network thread -> Update thread)
    moodycamel::ReaderWriterQueue<ConnectionManager::RawData> incoming_state_updates{}; //New state updates.(Network thread -> Update thread)
    moodycamel::ReaderWriterQueue<EventList> outgoing_events{}; //New input events.(Update thread -> Network thread)

    std::thread render_thread, network_thead; //Main thread is update thread.

    /**
     * Manage incoming server messages
     */
    void receiveCallback(bool TCP, const ConnectionManager::RawData& packet_data,ConnectionManager& manager){
        if(TCP){
            auto type = extractStructFromPacket<MessageTypeMetaData>(packet_data,0);
            if(type.type == NEW_OBJECT){
                incoming_objects.emplace(packet_data); //New object to instantiate
            }

        }else{  //data packet must be state update
            incoming_state_updates.emplace(packet_data);
        }
    }

    /**
     * Wait for incoming state and send out events
     */
    void networkThread() {
        { //do handshake
            ConnectionManager::RawData handshake_data;
            MessageTypeMetaData type_meta_data{HANDSHAKE};
            addStructToPacket(handshake_data, type_meta_data);
            HandShake hand_shake{PROTOCOL_VERSION};
            addStructToPacket(handshake_data, hand_shake);
            network.writeTCP(handshake_data);
        }
        while(running){
            //Gather messages
            network.processIncoming([this](bool TCP, const ConnectionManager::RawData& packet_data,ConnectionManager& manager){
                this->receiveCallback(TCP,packet_data,manager);
            },TICK_RATE,50);
            //Send the most recent event.
            EventList outgoing_event;
            if (outgoing_events.try_dequeue(outgoing_event)) {
                ConnectionManager::RawData data;
                ClientEvents events{network_counter,outgoing_event};
                addStructToPacket(data,events);
                network.writeUDP(data);
                network_counter = (network_counter + 1) % 256; //explicit wrap
            }
            while(outgoing_events.try_dequeue(outgoing_event)){} //Only send one event at a time.
        }
    }

    /**
     * Copy data and render it from the visibility buffer.
     */
    void renderThread(){

        while(running) {
            { //copy data
                std::lock_guard guard(visibility_buffer_mutex);
                for (int i = 0; i < MAX_VISIBLE_OBJECTS; ++i) {
                    render_buffer[i] = update_buffer[i] == nullptr ? nullptr : update_buffer[i]->copy();
                }
            }

            //Render
            {
                std::lock_guard guard(resource_mutex);
                for (int i = 0; i < MAX_VISIBLE_OBJECTS; ++i) {
                    if (render_buffer[i] != nullptr) {
                        //the camera is set by render method.
                        render_buffer[i]->render(renderer,
                                                 resource_manager);
                    }
                }
                renderer.getResult(frame_buffer);
            }
            window.drawFrameBuffer(frame_buffer);
        }
    }

public:

    /**
     * Create a client.
     * @warning While a server and a client can run on one machine simultaneously, two clients can not.
     * @param server_address Server to connect to.
     * @details Starts on port 8081.
     */
    explicit Client(const ConnectionManager::Address& server_address) :network(server_address,8081) {
        //Setup window
        window.setMouseRelative();
        //Start threads
        network_thead = std::thread(&Client::networkThread, this);
        render_thread = std::thread(&Client::renderThread, this);

        //Update thread

        EventList event;
        auto last_update = std::chrono::steady_clock::now(); //todo fix delta time(again)

        while (window.isOpen(event)) {

            //relay events
            outgoing_events.emplace(event);


            if(incoming_objects.size_approx() > 0){
                std::lock_guard guard(resource_mutex);
                //init new objects
                ConnectionManager::RawData new_object_data;
                while (incoming_objects.try_dequeue(new_object_data)) {
                    auto meta_data = extractStructFromPacket<NewObjectMetaData>(new_object_data,
                                                                                sizeof(MessageTypeMetaData));
                    object_cache[meta_data.object_id] = GameObject::instantiateGameObject(meta_data.type_id,
                                                                                          new_object_data,
                                                                                          sizeof(MessageTypeMetaData) +
                                                                                          sizeof(NewObjectMetaData));
                    {
                        //todo figure out why lock guard here causes issues
                        object_cache[meta_data.object_id]->loadResourcesClient(resource_manager, meta_data.is_associated);
                    }
                    object_cache[meta_data.object_id]->registerServices(services);
                }
            }


            //update state
            ConnectionManager::RawData new_state;
            while (incoming_state_updates.try_dequeue(new_state)) {
                auto meta_data = extractStructFromPacket<StateMetaData>(new_state,0);
                if(object_cache.find(meta_data.object_id) == object_cache.end()) continue; //Not instantiated yet
                std::lock_guard guard(visibility_buffer_mutex);
                object_cache[meta_data.object_id]->deserialize(new_state,sizeof(StateMetaData),meta_data.counter);
                assert(meta_data.buffer_location < MAX_VISIBLE_OBJECTS);
                update_buffer[meta_data.buffer_location] = object_cache[meta_data.object_id].get();
            }

            //delta time
            auto now = std::chrono::steady_clock::now();
            double delta_time = (double)std::chrono::duration_cast<std::chrono::microseconds>(now - last_update).count() / 1000000.0f;
            last_update = now;

            //predict

            for (auto &i: update_buffer) {
                std::lock_guard guard(visibility_buffer_mutex);
                if (i != nullptr) {
                    i->predict(delta_time, event, services, resource_manager);
                }
            }
        }
        stop();
    }

    /**
     * Stop the server if running.
     */
    void stop(){
        if(running){
            running = false;
            network_thead.join();
            render_thread.join();
        }
    }

    /**
     * Stops server,
     */
    ~Client(){
        stop();
    }

};
