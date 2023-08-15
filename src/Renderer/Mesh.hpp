//
// Created by Philip on 7/18/2023.
//

#pragma once

#include <vector>
#include "Triangle.hpp"

/**
 * A geometric construct consisting of triangles
 */
struct Mesh {
    std::vector<Triangle> tris;
};
