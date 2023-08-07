//
// Created by Philip on 7/21/2023.
//

#pragma once

#include "GameObject.hpp"
#include "../Physics/PhysicsMesh.hpp"

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
    SphereBV hitbox{{0,0,0},0.1};
    float current_radians_x = 0, current_radians_y = 0; //view
    Camera camera {90,{0,0,1},1};

    glm::vec3 position{2,2,2};
    glm::vec3 direction{1,0,0};
    glm::vec3 velocity{0,0,0};

    const float map_scale = 10.0f;

    bool main_player = false;

    ResourceManager::ResourceID texture;
    ResourceManager::ResourceID mesh;
public:
    void loadResourcesClient(ResourceManager &manager, bool associated) override {
        main_player = associated;
        texture = manager.getTexture("Shark.png");
        mesh = manager.getMesh("shorked.fbx", ResourceManager::FBX);
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

    void update(double delta_time, const EventList &events, const Services &services,
                const ResourceManager &resource_manager) override {
        //waste cpu cycles such that update and predict run long enough for delta_time to have any precision whatsoever.(Important for consistent movement betwen client and server)
        std::cout << "delta time " << delta_time << "\n";
            //todo collider
        direction = glm::normalize(direction);
        camera.setPosition(position);
        camera.setLookAt(position + direction);

        float speed = 10.0f;
        velocity = {0,0,0};

        //interpolate between polling
        if(events.buttons[0]){
            velocity += glm::vec3 {direction.x,direction.y,0} * speed;
        }
        if(events.buttons[1]){
            velocity += -glm::vec3 {direction.x,direction.y,0} * speed;
        }
        if(events.buttons[2]){
            velocity += -glm::vec3{direction.y,-direction.x,0}* speed;
        }
        if(events.buttons[3]){
            velocity += glm::vec3{direction.y,-direction.x,0}* speed;
        }
        if(events.buttons[4]){
            velocity += glm::vec3{0,0,-1}* speed;
        }
        if(events.buttons[5]){
            velocity += glm::vec3{0,0,1}* speed;
        }

        position += velocity * (float)delta_time;

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
        //nothing
    }

    void render(Renderer& renderer,FrameBuffer& frame_buffer, const ResourceManager& manager) const override {
            if(main_player){
                renderer.setCamera(camera);
            }else{
                renderer.draw(frame_buffer,manager.readMesh(mesh),glm::inverse(glm::lookAt(position,position+direction,{0,0,1})),manager.readTexture(texture));
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
        direction = state.direction;
    }

    std::unique_ptr<GameObject> createNewInternal(const PlayerArgs &params) const override {
        return std::unique_ptr<GameObject>(new Player());
    }

    void predictInternal(double delta_time, const EventList &events, const Services &services,
                         const ResourceManager &resource_manager) override {
        if(main_player){
            direction = glm::normalize(direction);
            camera.setPosition(position);
            camera.setLookAt(position + direction);

            float speed = 10.0f;
            velocity = {0,0,0};

            //interpolate between polling
            if(events.buttons[0]){
                velocity += glm::vec3 {direction.x,direction.y,0} * speed;
            }
            if(events.buttons[1]){
                velocity += -glm::vec3 {direction.x,direction.y,0} * speed;
            }
            if(events.buttons[2]){
                velocity += -glm::vec3{direction.y,-direction.x,0}* speed;
            }
            if(events.buttons[3]){
                velocity += glm::vec3{direction.y,-direction.x,0}* speed;
            }
            if(events.buttons[4]){
                velocity += glm::vec3{0,0,-1}* speed;
            }
            if(events.buttons[5]){
                velocity += glm::vec3{0,0,1}* speed;
            }

            position += velocity * (float)delta_time;

            const float SENSITIVITY = 100.0f;
            current_radians_x =  (float)events.mouse_x / SENSITIVITY;
            current_radians_y = (float)events.mouse_y / SENSITIVITY;
            direction.x = sinf(current_radians_x);
            direction.y = cosf(current_radians_x);

            const float VERTICAL_CLAMP = 1.5;

            //Clamp vertical
            if(current_radians_y > VERTICAL_CLAMP){
                current_radians_y = VERTICAL_CLAMP;
            }

            //Clamp vertical
            if(current_radians_y < -VERTICAL_CLAMP){
                current_radians_y = -VERTICAL_CLAMP;
            }

            direction.z = sinf(current_radians_y);


        } else{
            position += velocity * (float)delta_time;
        }
    }

    PlayerArgs getConstructorParamsInternal() const override {
        return PlayerArgs();
    }

};
  /*


    float grav_vel = 0; //Gravity velocity
   */
/*
       hitbox.position = position/map_scale;
        hitbox.radius = player_height/2.0f;

        direction = glm::normalize(direction);
        transform = glm::translate(glm::identity<glm::mat4>(),position);

        camera.setPosition(position);
        camera.setLookAt(position + direction);

        float speed = 100.0f * delta_time;

        glm::vec3  vel = {0,0,0};

        float down_dist;
        //todo scale and other raycast transforms
       if(map_collider.rayCast(position/map_scale,{0,0,1},down_dist)){
            if(down_dist < player_height){
                grav_vel = 0;
                position.z -= player_height - down_dist;
                if(input[4]){
                    grav_vel =  -3.0f;
                    vel += glm::vec3 {0,0,grav_vel} * speed;
                    input[4] = false;
                }
            }else{
                //position is not collision checked while vel is
                if(grav_vel > 0 ){
                    position += glm::vec3 {0,0,grav_vel} * speed; //no need for collision check here, raycast takes care of it.
                } else{
                    vel += glm::vec3 {0,0,grav_vel} * speed;    //Players should not clip through roof
                }
                //This also means players are less likely to get stuck on edges
               grav_vel += speed * 0.1f;
            }
        };

        if(input[5]) speed *= 2;

        //interpolate between polling
        if(input[0]){
            vel += glm::vec3 {direction.x,direction.y,0} * speed;
        }
        if(input[1]){
            vel += -glm::vec3 {direction.x,direction.y,0} * speed;
        }
        if(input[2]){
            vel += -glm::vec3{direction.y,-direction.x,0}* speed;
        }
        if(input[3]){
            vel += glm::vec3{direction.y,-direction.x,0}* speed;
        }

       for (const glm::vec3& collision_plane : map_collider.collide(hitbox)) {
           //collision plane is towards player
            glm::vec3 normal =  -collision_plane; //normal away from player
            float towards = abs(glm::dot(vel , normal)); //Get velocity towards plane
            vel -= towards * normal * 1.1f; //Make sure they cant stay in wall
       }

        position += vel;


        for (const SDL_Event& event : events.events) {
            if(event.type == SDL_KEYDOWN){
                switch (event.key.keysym.sym) {
                    case SDLK_w:
                        input[0] = true;
                        break;
                    case SDLK_s:
                        input[1] = true;
                        break;
                    case SDLK_a:
                        input[2] =  true;
                        break;
                    case SDLK_d:
                        input[3] =  true;
                        break;
                    case SDLK_SPACE:
                        input[4] =  true;
                        break;
                }
                if(event.key.keysym.mod == KMOD_LSHIFT){
                    input[5] = true;
                } else {
                    input[5] = false;
                }
            }else if(event.type == SDL_KEYUP){
                    switch (event.key.keysym.sym) {
                        case SDLK_w:
                            input[0] = false;
                            break;
                        case SDLK_s:
                            input[1] = false;
                            break;
                        case SDLK_a:
                            input[2] =  false;
                            break;
                        case SDLK_d:
                            input[3] =  false;
                            break;
                        case SDLK_SPACE:
                            input[4] =  false;
                            break;
                    }
                if(event.key.keysym.mod == KMOD_LSHIFT){
                    input[5] = true;
                } else {
                    input[5] = false;
                }

            } else if(event.type == SDL_MOUSEBUTTONDOWN){

            } else if(event.type == SDL_MOUSEMOTION){
                const float SENSITIVITY = 100.0f;
                current_radians_x +=  (float)event.motion.xrel / SENSITIVITY;
                current_radians_y += (float)event.motion.yrel / SENSITIVITY;
                direction.x = sinf(current_radians_x);
                direction.y = cosf(current_radians_x);

                const float VERTICAL_CLAMP = 1.5;

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
        }
 */