//
// Created by Philip on 7/21/2023.
//

#pragma once

#include <numeric>
#include <algorithm>
#include "../Renderer/Mesh.hpp"
#include "SphereBV.hpp"

/**
 * Mesh with BVH for physics
 * BVH uses Sphere BV
 */
class PhysicsMesh {
private:

   /**
    * Build BVH top down
    * @param current_triangles Current triangles to split
    * @return Index of built node
    */
    size_t recurseBuild(const std::vector<Triangle>& current_triangles) {
        BVNode node{};
        node.triangle = -1;
        if(current_triangles.size() == 1){
            node.bound = SphereBV(std::vector<glm::vec3>{current_triangles[0].pos[0],current_triangles[0].pos[1],current_triangles[0].pos[2]});
            node.triangle = current_triangles[0].index;
            bvh.push_back(node);
            return bvh.size()-1;
        }

        //split triangles
        std::vector<Triangle> a,b;
        split(current_triangles,a,b);

        //Create children
        node.child_a = (int) recurseBuild(a);
        node.child_b = (int) recurseBuild(b);

        //Get bounding sphere
        node.bound = SphereBV::combine({bvh[node.child_a].bound, bvh[node.child_b].bound});
        bvh.push_back(node);
        return bvh.size()-1;
    }

    /**
     * Get the center point of a triangle
    */
    static glm::vec3 getTriCenter(const Triangle& triangle){
        //todo optimize
        return (triangle.pos[0] + triangle.pos[1] + triangle.pos[2])/3.0f;
    }

    /**
     * Calculate the standard deviation of triangles along an axis
     * @param input Triangles
     * @param axis 0,1,or 2
     * @return Population standard deviation
     */
    static float triangleStandardDeviation(const std::vector<Triangle>& input, int axis){
        //Calculate mean
        float mean = std::accumulate(input.begin(), input.end(),0.0f, [axis] (float last_result, const Triangle& element){return last_result +  PhysicsMesh::getTriCenter(element)[axis];})/ (float)input.size();

        //subtract values and square
        float sum_of_squares = std::accumulate(input.begin(), input.end(),0.0f, [axis,mean] (float last_result, const Triangle& element){return last_result +  (PhysicsMesh::getTriCenter(element)[axis]-mean)*(PhysicsMesh::getTriCenter(element)[axis]-mean);});
        return sqrtf(sum_of_squares/(float)input.size());
    }


    /**
     * Split triangles into two optimal groups.
     * @param input Input triangles. 2 or more.
     * @param a,b Vectors to add split groups to.
     */
    static void split(const std::vector<Triangle>& input,std::vector<Triangle>& a, std::vector<Triangle>& b ){
        assert(input.size() > 1);
        if(input.size() == 2){ //Trivial
            a.push_back(input[0]);
            b.push_back(input[1]);
            return;
        }

        //Pick the best axis based on standard deviation
        int axis = 0;
        float best_deviation = triangleStandardDeviation(input,0);

        float deviation_y = triangleStandardDeviation(input,1);
        if(deviation_y > best_deviation){
            axis = 1;
            best_deviation = deviation_y;
        }

        float deviation_z = triangleStandardDeviation(input,2);
        if(deviation_z > best_deviation){
            axis = 2;
        }



        //sort by axis
        std::vector<Triangle> sorted = input;
        std::sort(sorted.begin(), sorted.end(), [axis]( const Triangle& lhs, const Triangle& rhs){return PhysicsMesh::getTriCenter(lhs)[axis] <  PhysicsMesh::getTriCenter(rhs)[axis];});

        a = std::vector(sorted.begin(),sorted.begin() + (sorted.size()/2));
        b = std::vector(sorted.begin() + (sorted.size()/2),sorted.end() );

    }



    /**
     * Traverse the BVH for ray casting
     */
    bool rayCastRecurse(const glm::vec3& origin, const glm::vec3& direction,float & distance, int index) const {
        float sphere_distance;
        if(bvh[index].bound.rayCast(origin,direction,sphere_distance)){
            if(bvh[index].triangle != -1){
                glm::vec3 barycentric;
                return mesh.tris[bvh[index].triangle].rayCast(origin,direction,distance,barycentric);
            }
            float distance_a;
            bool hit_a = rayCastRecurse(origin,direction,distance_a,bvh[index].child_a);
            float distance_b;
            bool hit_b = rayCastRecurse(origin,direction,distance_b,bvh[index].child_b);
            if(hit_a && (!hit_b || distance_a < distance_b)) {
                distance = distance_a;
                return hit_a;
            }
            if(hit_b){
                distance = distance_b;
                return hit_b;
            }
        }
        //todo optimize
        return false;
    }

    /**
     * Traverse the BVH for sphere collision
     */
    [[nodiscard]] std::vector<glm::vec3> collideRecurse(const SphereBV& sphere, int index) const {

        if(sphere.intersect(bvh[index].bound)){
            if(bvh[index].triangle != -1){
                glm::vec3 collision_point;
                glm::vec3 plane;
                if(sphere.collide( mesh.tris[bvh[index].triangle],collision_point,plane)){
                    return {plane};
                };
                return {};
            }

            //combine
            std::vector<glm::vec3> collide_a = collideRecurse(sphere,bvh[index].child_a);
            std::vector<glm::vec3> collide_b = collideRecurse(sphere,bvh[index].child_b);
            collide_a.insert(collide_a.end(),collide_b.begin(),collide_b.end());
            return collide_a;
        }
        //todo optimize
        return {};
    }


public:
    /**
     * Node in bvh tree
     */
    struct BVNode{
        SphereBV bound;
        int child_a,child_b; //Indexes in array
        int triangle; //-1 if not leaf. Index in mesh array
    };

/**
     * Create physics mesh from mesh
     * Will copy mesh and build BVH
     * @param source_mesh Mesh to use
     */
    explicit PhysicsMesh(const Mesh& source_mesh) : mesh(source_mesh){
        //Add indexes for BVH construction
        for (size_t i = 0; i < mesh.tris.size(); ++i) {
            mesh.tris[i].index = (int)i;
        }
        recurseBuild(mesh.tris);
    }

    PhysicsMesh() = default;

    /**
     * Ray cast the Physics mesh
     * @param origin Origin of ray
     * @param direction Direction of ray
     * @param distance Distance if hit
     * @return True if hit
     */
    bool rayCast(const glm::vec3& origin, const glm::vec3& direction,float & distance) const {
        return rayCastRecurse(origin,direction,distance,(int)bvh.size()-1);
    }

    /**
     * Collide a sphere with the mesh
     * @param sphere Sphere to collide
     * @return Triangle planes of triangles that collided
     */
    [[nodiscard]] std::vector<glm::vec3> collide(const SphereBV& sphere) const {
        return collideRecurse(sphere,(int)bvh.size()-1);
    }

    Mesh mesh;
    std::vector<BVNode> bvh; //Root node is last node
};
