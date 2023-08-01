//
// Created by Philip on 7/31/2023.
//

#pragma once


#include "GameObject.hpp"
#include "../Loaders/OBJLoader.hpp"
#include "../Loaders/TextureLoader.hpp"


class SDFDemo : public GameObject{
    std::vector<glm::vec3> spheres;
    Mesh mesh;
public:
    void setUp(std::vector<Texture> &textures) override {
        textures.push_back(loadTexture("test.png"));
        mesh = loadOBJ("sphere.obj", textures.size()-1);
        spheres.emplace_back(0,2,8);
        spheres.emplace_back(0,2,7);
        spheres.emplace_back(0,2,6);
        spheres.emplace_back(0,2,5);
        //spheres.emplace_back(0,2.2,4);
    }

    void update(double delta_time, const EventList &events) override {

        for (glm::vec3& sphere : spheres) {
            glm::vec3 velocity = {0,0,1.0 * delta_time};

            for (const glm::vec3& other_sphere : spheres) {
                if(other_sphere == sphere) continue;
                CollisionInfo info{};
                if(collideSDF([&sphere](const glm::vec3& point){
                    return sphereSDF(translateSDF(point,sphere), 0.5f);
                },[&other_sphere](const glm::vec3& point){
                    return sphereSDF(translateSDF(point,other_sphere), 0.5f);
                },sphere,other_sphere,info)){
                    //collision
                    glm::vec3 normal =  info.normal_a_to_b; //normal away from current sphere
                    float towards = abs(glm::dot(velocity , normal)); //Get velocity towards plane
                    velocity -= towards * normal * 1.1f; //Make sure they cant stay in wall
                }
            }
            //floor
            if(sphere.z >= 10){
                sphere.z =10;
                if(velocity.z > 0){
                    velocity.z = 0;
                }
            }
            sphere += velocity;
        }
    }

    void render(Renderer &renderer, FrameBuffer &frame_buffer) const override {
        for (const glm::vec3& sphere : spheres) {
            renderer.draw(frame_buffer,mesh,glm::translate(glm::identity<glm::mat4>(), sphere));
        }
    }

    SphereBV getBounds() const override {
        return SphereBV();
    }
};
