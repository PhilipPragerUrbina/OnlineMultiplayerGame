//
// Created by Philip on 7/19/2023.
//

#pragma once

#include <vector>
#include "Triangle.hpp"
#include "../GameState/Pose.hpp"

/**
 * Max number of bones that can influence a single vertex
 */
const int MAX_BONES_PER_VERTEX = 4;

/**
 * Triangle with additional skinning information
 */
struct SkinnedTriangle {
    Triangle triangle;
    int bone_ids[3][MAX_BONES_PER_VERTEX]; //-1 means no bone
    float weights[3][MAX_BONES_PER_VERTEX];
};

/**
 * A mesh that has skinning information for animations
 */
struct SkinnedMesh {
    std::vector<SkinnedTriangle> tris;
    int num_bones;
    std::vector<Pose> animations;
};
