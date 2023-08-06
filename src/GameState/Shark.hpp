//
// Created by Philip on 7/20/2023.
//

#pragma once


#include "GameObject.hpp"
#include "../Loaders/TextureLoader.hpp"
#include "../Loaders/FBXLoader.hpp"


/**
 * Test shark
 */
 /*
class Shark : public GameObject{
private:
    SkinnedMesh mesh;
    glm::mat4 transform = glm::identity<glm::mat4>();
    double time = 0;
public:

    SphereBV getBounds() const override {
        return SphereBV{};
    }

    Shark(const glm::mat4& transform= glm::identity<glm::mat4>()) : transform(transform){}

    void setUp(std::vector<Texture> &textures) override {
        textures.push_back(loadTexture("Shark.png"));
        mesh = loadFBXSkinned("shorked.fbx",textures.size()-1);
        mesh.animations[0].setDefaultPose();
    }

    void update(double delta_time, const EventList &events) override {
            for (const SDL_Event& event : events.events) {
                if(event.type == SDL_KEYDOWN){
                    mesh.animations[0].bones[1].current_transform = glm::rotate( glm::identity<glm::mat4>(), (float)sin(time*50.0)/30.0f , {0,0,1});
                    mesh.animations[0].bones[2].current_transform = glm::rotate(  glm::identity<glm::mat4>(), (float)sin(time*50.0)/30.0f , {0,0,1});
                    mesh.animations[0].bones[4].current_transform = glm::rotate(  glm::identity<glm::mat4>(), (float)sin(time*10.0)/10.0f , {0,1,0});
                }
            }
            transform = glm::rotate(transform, (float)(0.7 * delta_time), {0,1,0});
            time += delta_time;
    }

    void render(Renderer &renderer, FrameBuffer& frame_buffer) const override {
        renderer.drawSkinned(frame_buffer,mesh,transform, mesh.animations[0].computeGlobalTransforms() );
    }
};
*/