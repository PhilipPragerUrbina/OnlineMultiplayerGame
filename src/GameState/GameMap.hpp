//
// Created by Philip on 8/5/2023.
//

#pragma once


#include "GameObject.hpp"

struct MapState{
    int a = 0;
};

struct MapArgs{
    int b = 0;
};

class GameMap : public GameObjectImpl<GameMap,MapArgs,MapState>{
    uint16_t mesh;
    uint16_t texture;
public:
    void loadResourcesClient(ResourceManager &manager, bool associated) override {
        mesh = manager.getMesh("Map.obj",ResourceManager::OBJ);
        texture = manager.getTexture("Map.png");
    }

    void loadResourcesServer(ResourceManager &manager) override {
        //ignored
    }

    void registerServices(Services &services) override {
        //ignored
    }

    void deRegisterServices(Services &services) override {
            //ignore
    }

    void update(double delta_time, const EventList &events, const Services &services) override {
        //ignore
        camera.setPosition({0,0,0});
    }

    void updateServices(Services &services) const override {
        //ignore
    }

    Camera camera {90,{0,0,1},1};
    void render(moodycamel::ReaderWriterQueue<RenderRequest> &render_queue,
                moodycamel::ReaderWriterQueue<Camera> &camera_queue,bool last) const override {
        camera_queue.emplace(camera);
            render_queue.emplace(RenderRequest{texture,mesh,glm::identity<glm::mat4>(),{},last});
    }

    SphereBV getBounds() const override {
        return SphereBV();
    }

protected:
    MapState serializeInternal() const override {
        return MapState{};
    }

    void deserializeInternal(const MapState &state) const override {
        //ignored
    }

    std::unique_ptr<GameObject> createNewInternal(const MapArgs &params) const override {
        return  std::unique_ptr<GameObject>(new GameMap());
        //todo automate the createnew with simple constructor that takes in args and some casting like in copy()
    }

    void predictInternal(double delta_time, const EventList &events, const Services &services) override {
        //ignored
    }

    MapArgs getConstructorParamsInternal() const override {
        return {};
    }
};
