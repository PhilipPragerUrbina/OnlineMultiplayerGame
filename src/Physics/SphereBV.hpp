//
// Created by Philip on 7/21/2023.
//

#pragma once
#include <glm/glm.hpp>
#include <vector>
#include "glm/gtx/norm.hpp"

/**
 * A sphere bounding volume
 */
struct SphereBV {
    glm::vec3 position{};
    float radius;

    /**
     * Create a sphere bounding volume
     * @param position Position of sphere
     * @param radius Radius of sphere
     */
    SphereBV(const glm::vec3& position = {0,0,0},float radius = 0) : position(position), radius(radius) {}

    /**
     * Create an optimal sphere BV for a set of points
     * @param points Points to contain. Should not be empty.
     * @param inflate Additional extra radius to add.
     */
    explicit SphereBV(const std::vector<glm::vec3>& points, float inflate = 0){
        assert(!points.empty());
        //Get center of points using average
        glm::vec3 center = {0,0,0};
        for (const glm::vec3& point : points) {
            center += point;
        }
        center /= points.size();
        position = center;

        //Get maximum distance from center
        float max_dist = 0;
        for (const glm::vec3& point : points) {
            max_dist = std::max(max_dist,glm::distance(point,center));
        }
        radius = max_dist + inflate;
    }

    /**
   * Get the closest point on a line segment
   * @param a Start of line
   * @param b End of line
   * @param point Target point
   * @return Closest point
   */
    static glm::vec3 closestPointOnLineSegment(glm::vec3 a, glm::vec3 b, glm::vec3 point)
    {
        glm::vec3 ab = b - a;
        float t = glm::dot(point - a, ab) / glm::dot(ab, ab);
        return a + std::min(std::max(t, 0.0f), 1.0f) * ab;
    }


    //todo return proper collision normal
    /**
   * Collide this sphere with a triangle
   * @param collision_point Point of collision.
     * @param triangle_plane Normal of triangle surface toward collider.
   * @return True if intersect
   * @see https://wickedengine.net/2020/04/26/capsule-collision-detection/
   */
    [[nodiscard]] bool collide( const Triangle& other, glm::vec3& collision_point, glm::vec3& triangle_plane) const {
        glm::vec3 plane = glm::normalize(glm::cross(glm::vec3(other.pos[1] - other.pos[0]), glm::vec3(other.pos[2] - other.pos[0]))); //Get triangle plane

        if(glm::dot(glm::normalize(glm::vec3(other.pos[0]+other.pos[1]+other.pos[2])/3.0f - position),plane) > 0){ //If facing in same direction as the vector(position->triangle center)
           triangle_plane = -plane; //flip towards
        }else{
            triangle_plane = plane;
        }

        float distance = glm::dot(position - glm::vec3(other.pos[0]), plane); //Get sphere distance from plane
        if(distance < -radius || distance > radius) return false;
        glm::vec3 project_point = position - plane * distance;
        //Check if projected point is on triangle
        glm::vec3 c0 = glm::cross(project_point - glm::vec3(other.pos[0]), glm::vec3(other.pos[1]) - glm::vec3(other.pos[0]));
        glm::vec3 c1 = glm::cross(project_point - glm::vec3(other.pos[1]), glm::vec3(other.pos[2]) - glm::vec3(other.pos[1]));
        glm::vec3 c2 = glm::cross(project_point - glm::vec3(other.pos[2]), glm::vec3(other.pos[0]) - glm::vec3(other.pos[2]));
        collision_point = project_point;
        if(glm::dot(c0, plane) <= 0 && glm::dot(c1, plane) <= 0 && glm::dot(c2, plane) <= 0) return true;
        //Could still intersect with other point
        float radius2 = radius * radius;


        glm::vec3 point1 = closestPointOnLineSegment(glm::vec3(other.pos[0]), glm::vec3(other.pos[1]), position);
        glm::vec3 v1 = position - point1;
        float distsq1 = dot(v1, v1);
        bool intersects = distsq1 < radius2;


        glm::vec3 point2 = closestPointOnLineSegment(glm::vec3(other.pos[1]), glm::vec3(other.pos[2]), position);
        glm::vec3 v2 = position - point2;
        float distsq2 = dot(v2, v2);
        intersects |= distsq2 < radius2;


        glm::vec3 point3 = closestPointOnLineSegment(glm::vec3(other.pos[2]), glm::vec3(other.pos[0]), position);
        glm::vec3 v3 = position - point3;
        float distsq3 = dot(v3, v3);
        intersects |= distsq3 < radius2;

        if(intersects){
            glm::vec3 d = point2 - point1;
            float best_distsq = dot(d, d);
            glm::vec3 best_point = point1;

            d = position - point2;
            float distsq = dot(d, d);
            if(distsq < best_distsq)
            {
                best_distsq = distsq;
                best_point = point2;
            }

            d = position - point3;
            distsq = dot(d, d);
            if(distsq < best_distsq)
            {
                best_distsq = distsq;
                best_point = point3;
            }
            collision_point = best_point;
            return true;
        }
        return false;
    }



    /**
     * Expand sphere to encompass a point
     * @param inflate Additional extra radius to add beyond point.
     * @param point Point to contain
     */
    void expand(const glm::vec3& point,float inflate = 0){
        float distance = glm::distance(point,position);
        if(distance < radius) return;
        radius = distance+inflate;
    }

    /**
     * Test if two sphere BVs intersect
     * @return True if intersect
     */
    [[nodiscard]] bool intersect( const SphereBV& other) const {
        float combined_radius = radius + other.radius;
        float distance_squared = glm::distance2(position,other.position);
        return distance_squared < (combined_radius*combined_radius);
    }

    /**
     * Check if point is in sphere
     */
    [[nodiscard]] bool isPointContained(const glm::vec3& point) const{
        return glm::distance(point,position) < radius;
    }


    /**
     * Ray cast the sphere BV
     * @param origin Origin of ray
     * @param direction Direction of ray
     * @param distance Distance if hit
     * @return True if hit
     * Uses algorithm from: https://raytracing.github.io/books/RayTracingInOneWeekend.html
     */
    bool rayCast(const glm::vec3& origin, const glm::vec3& direction,float & distance) const {
        glm::vec3 oc = origin - position;
        float a = glm::dot(direction, direction);
        float b = 2.0f * glm::dot(oc, direction);
        float c =  glm::dot(oc, oc) - radius*radius;
        float discriminant = b*b - 4*a*c;
        distance = (-b - sqrtf(discriminant) ) / (2.0f*a); //todo don't run this if not hit
        return discriminant > 0;
    }


    /**
     * Combine multiple volumes into a single encompassing volume
     * @param spheres Spheres. Not empty.
     * @return Encompassing sphere
     */
    static SphereBV combine(const std::vector<SphereBV>& spheres){
        SphereBV parent{{0,0,0}, 0};
        assert(!spheres.empty());
        //Get center of spheres using average
        glm::vec3 center = {0,0,0};
        for (const SphereBV& sphere : spheres) {
            center += sphere.position;
        }
        center /= spheres.size();
        parent.position = center;

        //Get maximum distance from center
        float max_dist = 0;
        for (const SphereBV& sphere : spheres) {
            max_dist = std::max(max_dist,glm::distance(sphere.position,center)+sphere.radius); //add radius of sphere
        }
        parent.radius = max_dist;
        return parent;
    }
};
