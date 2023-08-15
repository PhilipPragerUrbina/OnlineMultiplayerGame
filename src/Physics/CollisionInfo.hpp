//
// Created by Philip on 7/31/2023.
//

#pragma once
#include "glm/glm.hpp"

/**
 * Info about a collision
 */
struct CollisionInfo {
    /**
     * World space collision point
     */
    glm::vec3 hit_point;
    /**
     * World space collision normal from the first object to the second object
     */
    glm::vec3 normal_a_to_b;
    /**
     * World space collision normal from the second object to the first object
     */
    glm::vec3 normal_b_to_a;
    //todo swap to single collision normal without hit point
    /**
     * Amount of overlap
     */
    float penetration_depth;
};
