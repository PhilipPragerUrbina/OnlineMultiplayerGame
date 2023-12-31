//
// Created by Philip on 8/5/2023.
//

#pragma once


#include "GameObject.hpp"

struct MapState{
};

struct MapArgs{
};

class GameMap : public GameObjectImpl<GameMap,MapArgs,MapState>{
    uint16_t mesh;
    uint16_t physics_mesh;
    uint16_t texture;
public:
    void loadResourcesClient(ResourceManager &manager, bool associated) override {
        mesh = manager.getMesh("vehicle_game/map.obj",ResourceManager::OBJ);
        physics_mesh =  manager.getPhysicsMesh(mesh);
        texture = manager.getTexture("vehicle_game/map_texture.png");

    }

    void loadResourcesServer(ResourceManager &manager) override {
        mesh = manager.getMesh("vehicle_game/map.obj",ResourceManager::OBJ);
        physics_mesh =  manager.getPhysicsMesh(mesh);
    }

    void registerServices(Services &services) override {
       services.map_service.registerMap(physics_mesh);
    }

    void deRegisterServices(Services &services) override {
            //ignore
    }

    void update(int delta_time, const EventList &events, const Services &services, const ResourceManager& resource_manager) override {
        //ignore
    }

    void updateServices(Services &services) const override {
        //ignore
    }


    void render(Renderer& renderer, const ResourceManager& manager) const override {
            renderer.queueDraw(manager.readMesh(mesh),glm::identity<glm::mat4>(),manager.readTexture(texture));
    }

    bool updateCamera(glm::vec3 &position, glm::vec3 &look_at) const override{
        return false;
    }

    SphereBV getBounds() const override {
        return SphereBV({0,0,0},10000.0f);
    }

protected:
    MapState serializeInternal() const override {
        return MapState{

        };
    }

    void deserializeInternal(const MapState &state) override {
        //ignore
    }

    std::unique_ptr<GameObject> createNewInternal(const MapArgs &params) const override {
        return  std::unique_ptr<GameObject>(new GameMap());
        //todo automate the createnew with simple constructor that takes in args and some casting like in copy()
    }

    void predict(int delta_time, const EventList &events, const Services &services, const ResourceManager& resource_manager) override {
    }

    MapArgs getConstructorParamsInternal() const override {
        return {};
    }
};
