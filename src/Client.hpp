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
 * Connects to server and runs game
 */
class Client {
private:

    static const int WIDTH = 800;
    static const int HEIGHT = 800;
    static const int MAX_VISIBLE_OBJECTS = 15;

    Window window{WIDTH,HEIGHT,"Client"};//(read Rendering thread & read Update thread)
    std::atomic<bool> running = true;//(read Rendering thread & write Update thread)

    ResourceManager resource_manager{}; //(read Rendering thread & write Update thread)
    std::mutex resource_mutex;

    moodycamel::ReaderWriterQueue<GameObject::RenderRequest> render_queue{}; // (Update thread -> Render thread)
    moodycamel::ReaderWriterQueue<Camera> camera_queue{}; // (Update thread -> Render thread)
    //todo remove camera queue
    //todo replace with double buffer instead of render queue


    FrameBuffer frame_buffer{WIDTH,HEIGHT,{0,0,0,0}}; //(write Rendering thread)
    Renderer renderer{Camera{90,{0,0,1}, (float)WIDTH / HEIGHT}}; //(write Rendering thread)

    std::unordered_map<uint16_t , std::unique_ptr<GameObject>> object_cache{}; //(write Update thread)
    GameObject* visible[MAX_VISIBLE_OBJECTS] = {nullptr}; //(write Update thread)
    Services services{};  //(write Update thread)

   ConnectionManager network; //(Network thread)

    moodycamel::ReaderWriterQueue<std::vector<uint8_t>> incoming_objects{}; //(Network thread -> Update thread)
    moodycamel::ReaderWriterQueue<std::vector<uint8_t>> incoming_state_updates{}; //(Network thread -> Update thread)
    moodycamel::ReaderWriterQueue<EventList> outgoing_events{}; // (Update thread -> Network thread)
    //todo check counters

    std::thread render_thread, network_thead; //Main thread is update thread.

    /**
     * Manage incoming server messages
     */
    void receiveCallback(bool TCP, const std::vector<uint8_t>& packet_data,ConnectionManager& manager){
        if(TCP){
            auto type = extractStructFromPacket<MessageTypeMetaData>(packet_data,0);

            if(type.type == NEW_OBJECT){
                incoming_objects.emplace(packet_data);
            }

        }else{  //data packet must be state update
            incoming_state_updates.emplace(packet_data);
        }
    }


    uint8_t network_counter = 0; //Used for UDP packet ordering
    /**
     * Wait for incoming state and send out events
     */
    void networkThread(){
        while(running){
            //Gather messages
            network.processIncoming([this](bool TCP, const std::vector<uint8_t>& packet_data,ConnectionManager& manager){
                this->receiveCallback(TCP,packet_data,manager);
            },tick_rate,50);

            EventList outgoing_event;
            if (outgoing_events.try_dequeue(outgoing_event)) {
                std::vector<uint8_t> data;
                ClientEvents events{network_counter,outgoing_event};
                addStructToPacket(data,events);
                network.writeUDP(data);
                network_counter = (network_counter + 1) % 256; //explicit wrap
            }
            while(outgoing_events.try_dequeue(outgoing_event)){} //clear events so far. todo find better container
        }
    }

    void renderThread(){
        while(running){
            //todo figure out thread sftey and mover back here

        }
    }

public:

    //todo doc
    explicit Client(const ConnectionManager::Address& server_address, ConnectionManager::Port client_port = 8081) :network(server_address,client_port) {
        //todo handshake
        window.setMouseRelative();
        network_thead = std::thread(&Client::networkThread, this);
        render_thread = std::thread(&Client::renderThread, this);

        EventList event;
        auto last_update = std::chrono::steady_clock::now();
        while (window.isOpen(event)) {

            //relay events
            outgoing_events.emplace(event);

            //init new objects
            std::vector<uint8_t> new_object_data;
            while (incoming_objects.try_dequeue(new_object_data)) {
                auto meta_data = extractStructFromPacket<NewObjectMetaData>(new_object_data,sizeof(MessageTypeMetaData));
                object_cache[meta_data.object_id] = GameObject::instantiateGameObject(meta_data.type_id,new_object_data,sizeof(MessageTypeMetaData) + sizeof(NewObjectMetaData));
                std::lock_guard guard(resource_mutex);
                object_cache[meta_data.object_id]->loadResourcesClient(resource_manager,meta_data.is_associated);
                object_cache[meta_data.object_id]->registerServices(services);
            }
            //update state
            std::vector<uint8_t> new_state;
            while (incoming_state_updates.try_dequeue(new_state)) {
                auto meta_data = extractStructFromPacket<StateMetaData>(new_state,0);
                if(object_cache.find(meta_data.object_id) == object_cache.end()) continue;
                object_cache[meta_data.object_id]->deserialize(new_state,sizeof(StateMetaData));
                assert(meta_data.buffer_location < MAX_VISIBLE_OBJECTS);
                visible[meta_data.buffer_location] = object_cache[meta_data.object_id].get();
            }

            auto now = std::chrono::steady_clock::now();
            double delta_time = (double)std::chrono::duration_cast<std::chrono::microseconds>(now - last_update).count() / 1000000.0f;
            last_update = now;
            //predict
            for (auto & i : visible) {
                if(i != nullptr){
                    i->predict(delta_time,event,services,resource_manager);
                }
            }
            renderer.clearFrame(frame_buffer, {0,0,0});
            for (auto & i : visible) {
                if(i != nullptr){
                   // std::lock_guard guard(resource_mutex);
                    i->render(renderer,frame_buffer,resource_manager); //Using first instead of last
                }
            }
            window.drawFrameBuffer(frame_buffer);

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
// todo thread pool to multi thread rendering of objects. List of frame buffers(one for each thread) are combined and displayed fast.