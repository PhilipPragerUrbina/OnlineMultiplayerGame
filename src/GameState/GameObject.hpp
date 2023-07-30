//
// Created by Philip on 7/19/2023.
//

#pragma once

#include "../Renderer/Renderer.hpp"
#include "../Events/EventList.hpp"
#include "../Physics/SphereBV.hpp"

/**
 * A game object in the game world
 * Does not need to have a transform, nor does it need to be a single mesh
 */
class GameObject {
public:

    /**
     * Set up your object here. It is recommended to load in your assets here.
     * @param textures Load any textures you need into here. Make sure to note down the texture id.
     */
    virtual void setUp(std::vector<Texture>& textures) = 0;
    //todo instancing
    //todo texture manager

    /**
     * This is called as often as possible for each object in an unspecified order
     * This is used to update game state such as transforms
     * @param events The current events such as player input
     * @param delta_time Time that the last frame took in milliseconds. Multiply by this to ensure consistent movement.
     */
    virtual void update(double delta_time, const EventList& events) = 0;

    /**
     * Use this to render your object every frame using draw() or drawSkinned().
     * This runs in a separate thread than update.
     * You may set the camera here if you want.
     * @warning No game state should be changed here. This is read-only,for thread safety reasons.
     * @param renderer The renderer to render your meshes with.
     * @param frame_buffer The screen to render to
     */
    virtual void render(Renderer& renderer, FrameBuffer& frame_buffer) const = 0;

    /**
     * Get bounds of object for ray casting and culling
     */
    virtual SphereBV getBounds() const = 0;

};
