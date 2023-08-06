//
// Created by Philip on 7/31/2023.
//

#pragma once
#include "CollisionInfo.hpp"
#include <functional>

/**
 * Represents a signed distance field
 * World space point in -> signed distance out.
 */
typedef std::function<float(const glm::vec3&)> SDF;

/**
 * Estimate the normal of the surface nearest to a point querying the SDF function
 * @param sdf Signed distance field
 * @param point Input point to SDF
 * @return Closest surface normal.
 */
glm::vec3 estimateSDFNormal(const SDF& sdf,const glm::vec3& point){
    const float NORMAL_ESTIMATION_DIST = 0.001;
    //probe axis
    float dist_center = sdf(point);
    float dist_z = sdf(point + glm::vec3 {0,0,NORMAL_ESTIMATION_DIST});
    float dist_y = sdf(point + glm::vec3 {0,NORMAL_ESTIMATION_DIST,0});
    float dist_x = sdf(point + glm::vec3 {NORMAL_ESTIMATION_DIST,0,0});
    //Use change to estimate normal
    return glm::normalize(glm::vec3 (dist_x - dist_center, dist_y- dist_center, dist_z - dist_center));
}

/**
 * Get next collision detection sample from last sample
 * @param a SDF 1
 * @param b SDF 2
 * @param last_sample Last sample taken.
 * @param orientation 0,1,2,3 Which axis to prefer when on a surface. Should be an enum. Todo make enum
 * @return New sample.
 */
glm::vec3 SDFSamplePropagate(const SDF& a, const SDF& b, const glm::vec3& last_sample, uint8_t orientation){
    //Compute distance
    float dist_a = a(last_sample);
    float dist_b = b(last_sample);

    //estimate normals
    glm::vec3 normal_a = estimateSDFNormal(a,last_sample);
    glm::vec3 normal_b = estimateSDFNormal(b,last_sample);
    //Calculate offsets to move toward close surfaces
    glm::vec3 offset = normal_a * -dist_a;
    offset +=  normal_a * -dist_a;
    offset *= 0.5f;
    //Apply perpendicular offset (boost samples to not get stuck)
    glm::vec3 normal_a_perp;
    glm::vec3 normal_b_perp;
    switch (orientation) {
        case 0:
            normal_a_perp = {-normal_a.y, normal_a.x, 0};
            normal_b_perp = {-normal_b.y, normal_b.x, 0};
            break;
        case 1:
            normal_a_perp = {normal_a.y, -normal_a.x, 0};
            normal_b_perp = {normal_b.y, -normal_b.x, 0};
            break;
        case 2:
            normal_a_perp = {-normal_a.x * normal_a.z, -normal_a.y * normal_a.z,
                        normal_a.x * normal_a.x + normal_a.y * normal_a.y};
            normal_b_perp = {-normal_b.x * normal_b.z, -normal_b.y * normal_b.z,
                        normal_b.x * normal_b.x + normal_b.y * normal_b.y};
            break;
        case 3:
            normal_a_perp = -glm::vec3 {-normal_a.x * normal_a.z, -normal_a.y * normal_a.z,
                         normal_a.x * normal_a.x + normal_a.y * normal_a.y};
            normal_b_perp = -glm::vec3{-normal_b.x * normal_b.z, -normal_b.y * normal_b.z,
                         normal_b.x * normal_b.x + normal_b.y * normal_b.y};
            break;
    }
    if(glm::dot(normal_a_perp,normal_b_perp) < 0) {
        normal_b_perp = -normal_b_perp; //Make sure both perpendicular vectors are facing in the same direction
    }
    float ratio = dist_a/dist_b; //Use more of that surface the farther away
    offset += normal_a_perp * (dist_a/ratio);
    offset += normal_b_perp * (dist_b*ratio);
    //Calculate new sample
    glm::vec3 new_sample = last_sample + offset;
    return new_sample;
}

/**
 * Run a collision test on two signed distance fields
 * @param a SDF 1 in world space
 * @param b SDF 2 in world space
 * @param center_a,center_b Centers of SDF body in world space.
 * @param info_out If collision detected, extra collision information will be put here.
 * @return True if collision detected.
 * @details Uses custom algorithm to approximate collision point.
 * @limitations Can produce inconsistent results, best used with an iterative solver. Currently, has a hard time detecting already deeply intersected objects.
 */
bool collideSDF(const SDF& a, const SDF& b, const glm::vec3& center_a, const glm::vec3& center_b, CollisionInfo& info_out){
    //Estimate collision plane to find a good starting point
    glm::vec3 collision_plane_center = (center_a + center_b) / 2.0f;

    const int NUM_ITERATIONS = 20; //Number of samples to take in each direction
    const float MIN_COLLISION_DISTANCE = 0.002; //Smallest distance to count as a collision
    glm::vec3 last_sample = SDFSamplePropagate(a,b,collision_plane_center,0);
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        //Check if collision was detected
        float dist_a = a(last_sample); //todo combine with dist check in normal and in propagate
        float dist_b = b(last_sample);
        if(dist_a <= MIN_COLLISION_DISTANCE && dist_b <= MIN_COLLISION_DISTANCE) {
            info_out.hit_point = last_sample;
            info_out.penetration_depth = -dist_a - dist_b;
            info_out.normal_a_to_b = estimateSDFNormal(a,last_sample);
            info_out.normal_b_to_a = estimateSDFNormal(b,last_sample);
            return true;
        }
        glm::vec3 last_sample_inner = SDFSamplePropagate(a,b,last_sample,2);
        for (int i_inner = 0; i_inner < NUM_ITERATIONS; ++i_inner) {
            //Check if collision was detected
            float dist_a_inner = a(last_sample_inner); //todo combine with dist check in normal and in propagate
            float dist_b_inner = b(last_sample_inner);
            if(dist_a_inner <= MIN_COLLISION_DISTANCE && dist_b_inner <= MIN_COLLISION_DISTANCE) {
                info_out.hit_point = last_sample_inner;
                info_out.penetration_depth = -dist_a_inner - dist_b_inner;
                info_out.normal_a_to_b = estimateSDFNormal(a,last_sample_inner);
                info_out.normal_b_to_a = estimateSDFNormal(b,last_sample_inner);
                return true;
            }
            //Calculate next sample
            last_sample_inner = SDFSamplePropagate(a,b,last_sample_inner,2);
        }
        //Calculate next sample
        last_sample = SDFSamplePropagate(a,b,last_sample,0);
    }
    last_sample = SDFSamplePropagate(a,b,collision_plane_center,1);
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        //Check if collision was detected
        float dist_a = a(last_sample); //todo combine with dist check in normal and in propagate
        float dist_b = b(last_sample);
        if(dist_a <= MIN_COLLISION_DISTANCE && dist_b <= MIN_COLLISION_DISTANCE) {
            info_out.hit_point = last_sample;
            info_out.penetration_depth = -dist_a - dist_b;
            info_out.normal_a_to_b = estimateSDFNormal(a,last_sample);
            info_out.normal_b_to_a = estimateSDFNormal(b,last_sample);
            return true;
        }
        glm::vec3 last_sample_inner = SDFSamplePropagate(a,b,last_sample,3);
        for (int i_inner = 0; i_inner < NUM_ITERATIONS; ++i_inner) {
            //Check if collision was detected
            float dist_a_inner = a(last_sample_inner); //todo combine with dist check in normal and in propagate
            float dist_b_inner = b(last_sample_inner);
            if(dist_a_inner <= MIN_COLLISION_DISTANCE && dist_b_inner <= MIN_COLLISION_DISTANCE) {
                info_out.hit_point = last_sample_inner;
                info_out.penetration_depth = -dist_a_inner - dist_b_inner;
                info_out.normal_a_to_b = estimateSDFNormal(a,last_sample_inner);
                info_out.normal_b_to_a = estimateSDFNormal(b,last_sample_inner);
                return true;
            }
            //Calculate next sample
            last_sample_inner = SDFSamplePropagate(a,b,last_sample_inner,3);
        }
        //Calculate next sample
        last_sample = SDFSamplePropagate(a,b,last_sample,1);
    }
    return false;
}

/**
 * Translate an input point before handing it to an SDF
 * @param point Initial point
 * @param translation Translation
 * @return Translated point ready to input into an SDF.
 * @details Simple world space to object space translation
 */
glm::vec3 translateSDF(const glm::vec3& point,const glm::vec3& translation){
    return point - translation;
}

/**
 * SDF for a sphere
 * @param point Input point
 * @param radius Sphere radius
 * @return Signed distance
 */
float sphereSDF(const glm::vec3& point, float radius){
    return glm::length(point)- radius;
}

//todo improve collision detection for deeply intersected objects
//todo write article on how it works