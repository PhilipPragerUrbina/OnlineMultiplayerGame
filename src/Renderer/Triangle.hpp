//
// Created by Philip on 7/18/2023.
//

#pragma once

#include "glm/glm.hpp"

/**
 * A mesh triangle
 */
struct Triangle {
    glm::vec4 pos[3]; //Vertex positions. Plus W component.
    glm::vec3 norm[3]; // Normals
    glm::vec2 tex[3]; //UV Coords
    int index; //Index in mesh for BVH

    /**
     * Ray cast the triangle
     * @param origin Origin of ray
     * @param direction Direction of ray
     * @param distance Distance if hit
     * @param barycentric_coordinates Barycentric coordinates if hit
     * @reference https://www.scratchapixel.com/lessons/3d-basic-rendering/ray-tracing-rendering-a-triangle/ray-triangle-intersection-geometric-solution.html
     * @return True if hit
     */
    bool rayCast(const glm::vec3& origin, const glm::vec3& direction,float & distance,glm::vec3& barycentric_coordinates) const {
        glm::vec3 v0v1 = glm::vec3(pos[1]) - glm::vec3(pos[0]);
        glm::vec3 v0v2 = glm::vec3(pos[2]) - glm::vec3(pos[0]);
        glm::vec3 pvec = glm::cross(direction,v0v2);
        float det = glm::dot(v0v1,pvec);

        //floating point error range. Larger for larger objects to avoid speckling problem.
        const float EPSILON = 0.000001f;
        if (fabsf(det) < EPSILON) return false;

        float invdet = 1.0f / det;
        glm::vec3 tvec = origin - glm::vec3(pos[0]);
        barycentric_coordinates[0] = glm::dot(tvec,pvec) * invdet;
        if (barycentric_coordinates[0] < 0.0f || barycentric_coordinates[0] > 1.0f) return false;

        glm::vec3 qvec = glm::cross(tvec,v0v1);
        barycentric_coordinates[1] = glm::dot(direction,qvec) * invdet;
        if (barycentric_coordinates[1] < 0.0f || barycentric_coordinates[0] + barycentric_coordinates[1] > 1.0f) return false;

        barycentric_coordinates[2] = 1.0f - barycentric_coordinates[0] - barycentric_coordinates[1];

        float dist = glm::dot(v0v2,qvec) * invdet;
        const float DELTA = 0.0001f; //check if in small range. this is to stop ray from intersecting with triangle again after bounce.
        if (dist > DELTA){
            distance = dist;
            return true;
        }
        return false;
    }
};

