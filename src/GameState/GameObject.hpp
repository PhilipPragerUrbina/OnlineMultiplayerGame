//
// Created by Philip on 7/19/2023.
//

#pragma once

#include <memory>
#include <typeindex>
#include "../Renderer/Renderer.hpp"
#include "../Physics/SphereBV.hpp"
#include "../Services/Services.hpp"
#include "../Loaders/ResourceManager.hpp"
#include "../Networking/PacketStructures.hpp"
#include "readwriterqueue/readerwriterqueue.h"

/**
 * Public wrapper for GameObjectImpl to hide template arguments.
 * @see GameObjectImpl
 */
class GameObject {
protected:
    /**
    * The type table contains an instance of each game object based on a unique type id.
    * These instances can then be copied to create new objects at runtime without any if ladders.
    */
    inline static std::unordered_map<uint16_t , std::unique_ptr<GameObject>> type_table{};
    /**
     * Reliably converts large c++ type index to a small uint type id for sending over the network.
     */
    inline static std::unordered_map<std::type_index, uint16_t> type_id_table{};
    inline static uint16_t last_available_type_id = 0;
public:
    /**
     * Create a new game object.
     * @param type_id Game object type.
     * @param packet Raw packet data containing constructor parameter struct.
     * @param begin Byte where the constructor parameter struct starts in the packet.
     * @return New game object.
     */
    static std::unique_ptr<GameObject> instantiateGameObject(uint16_t type_id, const std::vector<uint8_t>& packet,size_t begin ){
        return type_table[type_id].get()->createNew(packet,begin);
    }

    /**
     * Get the type id of a game-object.
     * @return unsigned int type id.
     */
    [[nodiscard]] virtual uint16_t getTypeID() const = 0;

    /**
     * Get a copy of the game object.
     */
    [[nodiscard]] virtual std::unique_ptr<GameObject> copy() const = 0;

    /**
    * Deserialize a packet and use it to create a new instance of this type.
    * @param packet Packet containing constructor parameters struct.
    * @param begin Byte where the struct begins in data.
    * @return New separate instance of this class.
    */
    [[nodiscard]] virtual std::unique_ptr<GameObject> createNew(const std::vector<uint8_t>& packet, size_t begin) const = 0;

    /**
     * Append the data that this object wants to send to the client to a packet.
     * @param packet Packet to append data to.
     */
    virtual void serialize(std::vector<uint8_t>& packet) const = 0;

    /**
     * Use packet data to update game object state.
     * @param packet Packet containing state struct.
     * @param begin Byte where the state struct begins in data.
     * @warning Assumes the game object type has already been verified.
     * @warning Should only be called by network thread.
     */
    virtual void deserialize(const std::vector<uint8_t>& packet, size_t begin) = 0;

    /**
     * Load in textures, meshes, physics meshes. (For the client)
     * @param textures Use the get[type] methods to load your assets from a file, and then store the resulting id for use later.
     * @param associated Is this object associated with this specific client?
     */
    virtual void loadResourcesClient(ResourceManager& manager, bool associated) = 0;


    /**
     * Load in assets here (Usually just physics meshes). (For the server)
     * @param textures Use the get[type] methods to load your assets from a file, and then store the resulting id for use later.
     */
    virtual void loadResourcesServer(ResourceManager& manager) = 0;

    /**
     * Register the game object with any relevant services here
     */
    virtual void registerServices(Services& services) = 0;

    /**
     * deRegister the game object with any registered services here
     */
    virtual void deRegisterServices(Services& services) = 0;
    //todo auto detect

    /**
     * This is called as often as possible on the server for each object in an unspecified order.
     * @details This is used to update game state.
     * @param services Use this to query services. This is for reading only, see updateServices() for writing to services.
     * @param events Player events. (Only if an object is associated with a player).
     * @param delta_time Time that the last frame took in milliseconds. Multiply by this to ensure consistent movement.
     */
    virtual void update(double delta_time, const EventList& events, const Services& services) = 0;

    /**
     * Like update, but it runs on the client side.
     * @param delta_time Time that the last frame took in milliseconds. Multiply by this to ensure consistent movement.
     * @param events Events that the client is inputting.
     * @param services Use this to query services. This is for reading only, see updateServices() for writing to services.
     * Will predict game state and apply new server state.
     */
    virtual void predict(double delta_time, const EventList& events, const Services& services) = 0;

    /**
     * Write to services here to update them on the state of the game object.
     * @see update() and predict() for querying services and updating the game object.
     */
    virtual void updateServices(Services& services) const = 0;


    struct RenderRequest {
        ResourceManager::ResourceID  texture;
        ResourceManager::ResourceID  mesh;
        glm::mat4 model_transform;
        std::vector<glm::mat4> bones;
        bool last; //last visible object for this frame
    };
    /**
     * Use this to render your object every frame using queueDraw() or queueDrawSkinned().
     * You may set the camera here if you want, but only one game object on the client should do this.
     * @warning No game state should be changed here. This is read-only,for thread safety reasons.
     * @param renderer The renderer to render your meshes with.
     */
    virtual void render( moodycamel::ReaderWriterQueue<RenderRequest>& render_queue, moodycamel::ReaderWriterQueue<Camera>& camera_queue,bool last) const = 0;
    //todo change to better and doc

    /**
     * Get bounding-sphere of this object for ray casting and culling
     */
    [[nodiscard]] virtual SphereBV getBounds() const = 0;

    /**
    * Append the data that is needed to create a new instance of it on the client.
    * @param packet Packet to append data to.
    */
    virtual void getConstructorParams(std::vector<uint8_t>& packet) const = 0;

    virtual ~GameObject()= default;

};

/**
 * Base class for a game object in the game world
 * Does not need to have a transform, nor does it need to be a single mesh.
 * @details What is considered a single game object depends on two factors.
 * Spatial Coherency: If objects are connected to each other, they can be considered to be one game object. Such as a player, their camera, and their gun. This is to allow frustum culling.
 * Packets: If the state of the game object is sent over the network, each game object has ONE packet. So a gameobjects state must be serializable in less than a certain number of bytes
 * and usually with a known size (No variable length arrays and such).
 * If it does not meet these criteria, it must be split into multiple game objects.
 * @tparam SELF The type of the child class
 * @tparam CONSTRUCTION_PARAMS The struct containing data needed to construct a new instance of the game object.
 * Should not be the same things as in the state, this is for parameters that last the entire lifetime of the object and are not changed.
 * @tparam STATE A struct containing state that will be sent in a packet. Should not be very large and should only contain what the client needs to know. It Can be empty as well.
 * @warning Since these objects are heavily copied and serialized, pointers should be used with caution.
 * @see GameObject
 */
template <class SELF, class CONSTRUCTION_PARAMS, class STATE> class GameObjectImpl : public GameObject{
private:
    /**
     * Used to register the type in the type table.
     */
    template <class SELF_INNER> struct StaticTypeConstructor {
        StaticTypeConstructor(){
            std::cerr << "Object initialized";
            type_table[last_available_type_id] = std::unique_ptr<GameObject>(new SELF_INNER()); //Must have default constructor.
            type_id_table[std::type_index(typeid(SELF_INNER))] = last_available_type_id;
            last_available_type_id++;
        }
        //No destructor needed thanks to the unique pointer.
    };
    //Should be specialized such that it is a separate static instance for each game object type.
    inline static StaticTypeConstructor<SELF> static_type_constructor{};

    // Keep a FIFO style buffer between network thread and prediction thread.
    STATE state_buffer;//Written by network thread and read by prediction thread.
    //Check for changes in thread safe way.
    int state_update_count = 0; //Written by network thread and read by prediction thread.
    int last_state_update_count = 0; //Read and write by prediction thread.

    /**
     * @warning Should only be called by prediction thread
     * Transfer server state to override prediction state(If available).
     */
    void sync(){
        static_type_constructor; //use it or lose it. todo make sure this is not optimized away
        if(state_update_count != last_state_update_count){
            deserializeInternal(state_buffer);
            last_state_update_count++;
        }
    }

protected:



    /**
     * @return Struct containing any state the client must know.
     * @warning Must be smaller than the maximum packet size minus the auto-included metadata.
     */
    virtual STATE serializeInternal() const = 0;


    /**
     * @param state The state to apply to the object.
     */
    virtual void deserializeInternal(const STATE& state) const = 0;

    /**
       * Create a new instance of this type.
       * @details Basically a copy, but without actually copying the state, just constructing an object through another.
       * @param params Parameters for object.
       * @return New separate instance of this class.
       */
    virtual std::unique_ptr<GameObject> createNewInternal(const CONSTRUCTION_PARAMS& params) const = 0;

    /**
     * Like update, but it runs on the client side.
     * @param delta_time Time that the last frame took in milliseconds. Multiply by this to ensure consistent movement.
     * @param events Events that the client is inputting.
     * @param services  Use this to query services. This is for reading only, see updateServices() for writing to services.
     * Use this to update the game state, but will be overridden by server state when available.
     */
    virtual void predictInternal(double delta_time, const EventList& events, const Services& services) = 0;

    /**
     * Get the constructor params that can be used to create a copy of this object on the client side.
     * @return Constructor params originally used to instantiate this object.
     */
    virtual CONSTRUCTION_PARAMS getConstructorParamsInternal() const = 0;
public:

    [[nodiscard]] uint16_t getTypeID() const override {
        return type_id_table[typeid(SELF)];
    }

    [[nodiscard]] std::unique_ptr<GameObject> copy() const override {
        return std::unique_ptr<GameObject>(new SELF(*((SELF*)this)));
    }

    //todo example file showing steps to implement game object

    /**
     * @warning The child must implement a default constructor.
     */
    GameObjectImpl() = default;


    [[nodiscard]] std::unique_ptr<GameObject> createNew(const std::vector<uint8_t>& packet, size_t begin) const override {
        CONSTRUCTION_PARAMS params = extractStructFromPacket<CONSTRUCTION_PARAMS>(packet,begin);
        return createNewInternal(params);
    }


    void getConstructorParams(std::vector<uint8_t>& packet) const override {
        CONSTRUCTION_PARAMS params = getConstructorParamsInternal();
        addStructToPacket(packet,params);
    }

    void serialize(std::vector<uint8_t>& packet) const override {
        STATE state = serializeInternal();
        addStructToPacket(packet,state);
    }

    void deserialize(const std::vector<uint8_t>& packet, size_t begin) override {
        state_buffer = extractStructFromPacket<STATE>(packet,begin);
        state_update_count++;
    }


    void predict(double delta_time, const EventList& events, const Services& services) override {
        predictInternal(delta_time,events, services);
        sync();
    }

    ~GameObjectImpl() override= default;

    //todo audio

};
