//
// Created by Philip on 7/21/2023.
//

#pragma once

#include "GameObject.hpp"
#include "../Physics/PhysicsMesh.hpp"
#include "glm/gtx/rotate_vector.hpp"

struct PlayerArgs {

};

struct PlayerState{
    glm::vec3 position;
    glm::vec3 direction;
    glm::vec3 velocity;
};

/**
 * The player controller
 */
class Player : public GameObjectImpl<Player,PlayerArgs,PlayerState>{
private:
    SphereBV hitbox{{0,0,0},0.3};
    float current_radians_x = 0, current_radians_y = 0; //view
    Camera camera {90,{0,0,1},1};

    glm::vec3 position{2,2,2};
    glm::vec3 direction{1,0,0};
    glm::vec3 velocity{0,0,0};

    const float map_scale = 10.0f;
    float grav_vel = 0; //Gravity velocity
    bool main_player = false;

    ResourceManager::ResourceID texture;
    ResourceManager::ResourceID mesh;
public:
    void loadResourcesClient(ResourceManager &manager, bool associated) override {
        main_player = associated;
        texture = manager.getTexture("Shark.png");
        mesh = manager.getMesh("shorked.fbx", ResourceManager::FBX);
        camera.setPosition({2,2,2});
    }

    void loadResourcesServer(ResourceManager &manager) override {
        //nothing
    }

    void registerServices(Services &services) override {
        //nothing
    }

    void deRegisterServices(Services &services) override {
        //nothing
    }

    void update(int delta_time, const EventList &events, const Services &services,
                const ResourceManager &resource_manager) override {
        //waste cpu cycles such that update and predict run long enough for delta_time to have any precision whatsoever.(Important for consistent movement betwen client and server)
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
            //todo collider

        direction = glm::normalize(direction);
        hitbox.position = position/map_scale;
        hitbox.radius = 0.5/2.0f;

        camera.setPosition(position);
        camera.setLookAt(position + direction);

        float speed = 0.01f;
        velocity = {0,0,0};

        const float player_height = 0.5;
        float down_dist;
        //todo scale and other raycast transforms
        if(resource_manager.readPhysicsMesh(services.map_service.queryCollider())->rayCast(position/map_scale,{0,0,1},down_dist)){
            if(down_dist < player_height){
                grav_vel = 0;
                position.z -= player_height - down_dist;
               // if(events.keys[4]){
                  //  grav_vel =  -30.0f;
                  //  velocity += glm::vec3 {0,0,grav_vel} * (float)delta_time;
               // }
            }else{
                //position is not collision checked while vel is
                if(grav_vel > 0 ){
                    position += glm::vec3 {0,0,grav_vel} *  (float)delta_time; //no need for collision check here, raycast takes care of it.
                } else{
                    velocity += glm::vec3 {0,0,grav_vel} *  (float)delta_time;    //Players should not clip through roof
                }
                //This also means players are less likely to get stuck on edges
                grav_vel +=  0.0001f;
            }
        };

        //interpolate between polling
        if(events.keys[0]){
            velocity += glm::vec3 {direction.x,direction.y,0} * speed;
        }
        if(events.keys[1]){
            velocity += -glm::vec3 {direction.x,direction.y,0} * speed;
        }
        if(events.keys[2]){
            velocity += -glm::vec3{direction.y,-direction.x,0}* speed;
        }
        if(events.keys[3]){
            velocity += glm::vec3{direction.y,-direction.x,0}* speed;
        }
        if(events.keys[4]){
           // velocity += glm::vec3{0,0,-10}* speed;
        }
        if(events.keys[5]){
           // velocity += glm::vec3{0,0,1}* speed;
        }
        for (const glm::vec3& collision_plane :resource_manager.readPhysicsMesh(services.map_service.queryCollider())->collide(hitbox)) {
            //collision plane is towards player
            glm::vec3 normal =  -collision_plane; //normal away from player
            float towards = abs(glm::dot(velocity , normal)); //Get velocity towards plane
            velocity -= towards * normal * 1.1f; //Make sure they cant stay in wall
        }

        position += velocity*(float)delta_time;

        const float SENSITIVITY = 100.0f;
        current_radians_x =  (float)events.mouse_x / SENSITIVITY;
        current_radians_y = (float)events.mouse_y / SENSITIVITY;
        direction.x = sinf(current_radians_x);
        direction.y = cosf(current_radians_x);

        const float VERTICAL_CLAMP = 1.5; //todo vertical clampp does not actually effect event mouse and causes delay

        //Clamp vertical
        if(current_radians_y > VERTICAL_CLAMP){
            current_radians_y = VERTICAL_CLAMP;
        }

        //Clamp vertical
        if(current_radians_y < -VERTICAL_CLAMP){
            current_radians_y = -VERTICAL_CLAMP;
        }

        direction.z = sinf(current_radians_y);



    }

    void updateServices(Services &services) const override {
        services.map_service.chaser = position;
        //nothing
    }

    void render(Renderer& renderer, const ResourceManager& manager) const override {
            if(main_player){
                renderer.setCamera(camera);
            }else{
                renderer.queueDraw(manager.readMesh(mesh),glm::translate(glm::identity<glm::mat4>(),position)* glm::rotate(glm::orientation(glm::vec3 {direction.x,direction.y,direction.z},{0,1,0}), 3.1415f/2.0f, {0,0,-1}),manager.readTexture(texture));
            }
    }
  
    SphereBV getBounds() const override {
        return hitbox;
    }

protected:
    PlayerState serializeInternal() const override {
        return PlayerState{position,direction,velocity};
    }

    void deserializeInternal(const PlayerState &state) override {
        position = state.position;
        velocity = state.velocity;
        if(!main_player){ //Since looking is deterministic the main player can be authoritative
            direction = state.direction;
        }
        camera.setPosition(position);
        camera.setLookAt(position + direction);
        hitbox.position = position/map_scale;
    }

    std::unique_ptr<GameObject> createNewInternal(const PlayerArgs &params) const override {
        return std::unique_ptr<GameObject>(new Player());
    }


    void predict(int delta_time, const EventList &events, const Services &services,
                 const ResourceManager &resource_manager) override {
        if(main_player){
            update(delta_time,events,services,resource_manager);


        } else{
            position += velocity * (float)delta_time;
        }
    }

    PlayerArgs getConstructorParamsInternal() const override {
        return PlayerArgs();
    }

};

