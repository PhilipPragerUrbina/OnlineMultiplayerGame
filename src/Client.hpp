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
    Renderer renderer {WIDTH,HEIGHT}; //Main rendering engine.(Write Rendering thread)

    std::unordered_map<uint16_t , std::unique_ptr<GameObject>> object_cache{}; //Contains instantiated objects.(Write Update thread)
    Services services{};  //Allows game objects to communicate.(Write Update thread)

    ConnectionManager network; //Connects with server.(Write Network thread)
    uint8_t network_counter = 0; //Used for UDP packet ordering.(Write Network thread)

    moodycamel::ReaderWriterQueue<ConnectionManager::RawData> incoming_objects{}; //New objects to instantiate.(Network thread -> Update thread)
    moodycamel::ReaderWriterQueue<ConnectionManager::RawData> incoming_state_updates{}; //New state updates.(Network thread -> Update thread)
    moodycamel::ReaderWriterQueue<ClientEvents> outgoing_events{}; //New input events.(Update thread -> Network thread)

    std::thread render_thread, network_thead; //Main thread is update thread.

    //belongs to render thread
    Camera global_camera{90,{0,0,-1},1}; //todo allow aspect ratio to change and update server values

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
            if (!network.writeTCP(handshake_data)) {
                std::cerr << "Server handshake failed \n";
                //todo retry
            };
        }
        {   //set camera on server
            ConnectionManager::RawData camera_data;
            MessageTypeMetaData type_meta_data{CAMERA_CHANGE};
            addStructToPacket(camera_data, type_meta_data);
            CameraChange camera_change{90,1};
            addStructToPacket(camera_data, camera_change);
            if(!network.writeTCP(camera_data)){
                std::cerr << "Server camera set failed \n";
            };

        }
        while(running){

            //Gather messages
            if(!network.processIncoming([this](bool TCP, const ConnectionManager::RawData& packet_data,ConnectionManager& manager){
                this->receiveCallback(TCP,packet_data,manager);
            },TICK_RATE,50)){
                std::cerr << "Server disconnected \n";
                //todo rejoin server if disconnected
            }

            //Send the most recent event.
            ClientEvents outgoing_event;
            while(outgoing_events.try_dequeue(outgoing_event)){} //Only send one event at a time.

            ConnectionManager::RawData data;
            outgoing_event.counter = network_counter;
            addStructToPacket(data,outgoing_event);
            network.writeUDP(data);
            network_counter = (network_counter + 1) % 256; //explicit wrap

            std::this_thread::sleep_for(std::chrono::milliseconds (TICK_RATE)); //I know this doesn't actually enforce tick rate, but it doesn't really matter.
        }
    }

    /**
     * Copy data and render it from the visibility buffer.
     */
    void renderThread(){
        while(running) {
            { //copy data
                std::lock_guard guard(visibility_buffer_mutex);
                for (uint32_t i = 0; i < MAX_VISIBLE_OBJECTS; ++i) {
                    render_buffer[i] = update_buffer[i] == nullptr ? nullptr : update_buffer[i]->copy();
                }
            }
            {
                //set camera
                glm::vec3 new_position = {2,2,2}; //differ default values to avoid nans
                glm::vec3 new_look_at = {0,0,0};
                for (uint32_t i = 0; i < MAX_VISIBLE_OBJECTS; ++i) {
                    if (render_buffer[i] != nullptr) {
                        if(render_buffer[i]->updateCamera(new_position,new_look_at)){
                            global_camera.setPosition(new_position);
                            global_camera.setLookAt(new_look_at);
                            break;
                        }
                    }
                }
                renderer.setCamera(global_camera);

                std::lock_guard guard(resource_mutex); //Resources should not be edited while in use by renderer.
                //Queue draw calls
                for (uint32_t i = 0; i < MAX_VISIBLE_OBJECTS; ++i) {
                    if (render_buffer[i] != nullptr) {
                        if(render_buffer[i]->getBounds().inFrustum(global_camera)){
                            render_buffer[i]->render(renderer,resource_manager);
                        }
                    }
                }
                //Render the image (Still holds pointers to resources)
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
        auto last_update = std::chrono::steady_clock::now();

        while (window.isOpen(event)) {

            //delta time
            auto now = std::chrono::steady_clock::now();
            auto delta_time = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_update).count();
            last_update = now;

            //relay events
            outgoing_events.emplace(ClientEvents{0,(uint16_t)delta_time,event});

            //init new objects
            ConnectionManager::RawData new_object_data;
            while (incoming_objects.try_dequeue(new_object_data)) {
                auto meta_data = extractStructFromPacket<NewObjectMetaData>(new_object_data,sizeof(MessageTypeMetaData));
                object_cache[meta_data.object_id] = GameObject::instantiateGameObject(meta_data.type_id,new_object_data,sizeof(MessageTypeMetaData) + sizeof(NewObjectMetaData));
                {
                    std::lock_guard guard(resource_mutex);
                    object_cache[meta_data.object_id]->loadResourcesClient(resource_manager, meta_data.is_associated);
                }
                object_cache[meta_data.object_id]->registerServices(services);
            }

            //update state
            {
                std::lock_guard guard(visibility_buffer_mutex);
                ConnectionManager::RawData new_state;
                while (incoming_state_updates.try_dequeue(new_state)) {
                    auto meta_data = extractStructFromPacket<StateMetaData>(new_state, 0);
                    if (object_cache.find(meta_data.object_id) == object_cache.end()) continue; //Not instantiated yet
                    object_cache[meta_data.object_id]->deserialize(new_state, sizeof(StateMetaData));
                    update_buffer[meta_data.buffer_location] = object_cache[meta_data.object_id].get();
                }
            }

            //todo delete server deleted objects

            //predict
            for (auto &i: update_buffer) {
                if (i != nullptr) {
                    std::lock_guard guard(visibility_buffer_mutex);
                    i->predict(delta_time, event, services, resource_manager);
                }
            }
        }
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
