//
// Created by Philip on 7/21/2023.
//

#pragma once

#include "GameObject.hpp"
#include "../Physics/PhysicsMesh.hpp"

/**
 * The player controller
 */
 /*
class Player : public GameObject{
private:

    glm::mat4 transform = glm::identity<glm::mat4>();
    Camera camera;

    glm::vec3 direction;
    glm::vec3 position;

    bool input[6] = {false, false, false, false, false, false};

    float current_radians_x = 0, current_radians_y = 0; //view

    Mesh map_mesh;
    PhysicsMesh map_collider;

    SphereBV hitbox;

    const float map_scale = 1.0f;
    const float player_height = 10.0f;

    float grav_vel = 0; //Gravity velocity
public:

    [[nodiscard]] SphereBV getBounds() const override {
        return hitbox;
    }

    Player(const glm::vec3 & position, const Camera& camera) : camera(camera), direction({1,0,0}), position(position),
                                                               hitbox(position,0.3f){}

    void setUp(std::vector<Texture> &textures) override {
        textures.push_back(loadTexture("Map2.png"));
        map_mesh = loadOBJ("Map2.obj", textures.size()-1);
        map_collider = PhysicsMesh(map_mesh);

        position += glm::vec3 {0,0,-2};
    }

    void update(double delta_time, const EventList &events) override {
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

    }

    void render(Renderer &renderer, FrameBuffer& frame_buffer) const override {
        renderer.draw(frame_buffer,map_mesh, glm::scale(glm::identity<glm::mat4>(), {map_scale,map_scale,map_scale}));
        renderer.setCamera(camera);
    }
};
  */
//todo rewrite and create map class
//todo make sure to only set camera for primary player