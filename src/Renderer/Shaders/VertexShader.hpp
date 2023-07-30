//
// Created by Philip on 7/18/2023.
//

#pragma once

#include "../Triangle.hpp"
#include "../Camera.hpp"
#include "../SkinnedMesh.hpp"
#include <glm/gtx/string_cast.hpp>
/**
 * Transforms vertices
 */
class VertexShader {
private:
    //todo interface version
    //Seperate camera matrix
    glm::mat4 camera_transform;
    glm::mat4 camera_projection;
    glm::mat4 clip_matrix; //camera matrix * transform
    glm::mat3 normal_matrix; //For correcting normals
public:

    /**
     * Set the model transform of the shader
     * Do this once for each 3d model with a different transform
     */
    void setModelTransform(const glm::mat4& transform){
        clip_matrix = camera_transform * transform;
        normal_matrix = glm::mat3(glm::transpose(glm::inverse(transform)));
        //see https://learnopengl.com/Lighting/Basic-Lighting
    }

    /**
     * Set the camera the shader should use
     * Do this every time the camera moves or changes
     */
    void setCamera(const Camera& camera){
        camera_transform = camera.getTransform();
        camera_projection = camera.getProjection();
    }

    /**
     * Transform a triangle from model space into view space
     * @param model_space Input triangle
     * @return View space triangle
     * Make sure to set the camera and model matrix beforehand
     */
    [[nodiscard]] Triangle toViewSpace(const Triangle& model_space) const {
        Triangle view_tri{};
        view_tri.texture_id = model_space.texture_id;
        for (int i = 0; i < 3; ++i) {
            view_tri.pos[i] = clip_matrix * model_space.pos[i];
            view_tri.norm[i] = normal_matrix * model_space.norm[i];
            view_tri.tex[i] = model_space.tex[i];
        }
        return view_tri;
    }

    /**
     * Transform a triangle from view space to clip space by projecting it
     * @param view_space View space triangle
     * @return Clip space triangle
     */
    [[nodiscard]] Triangle toClipSpace(const Triangle& view_space){
        Triangle clip_tri = view_space;
        for (int i = 0; i < 3; ++i) {
            clip_tri.pos[i] = camera_projection * view_space.pos[i];
        }
        return clip_tri;
    }

    /**
     * Transform a triangle from model space into view space and deform with bones
     * @param model_space Skinned triangle
     * @param bones Final Skeleton pose transforms
     * @return View space triangle
     * Make sure to set the camera and model matrix beforehand
     * @details https://learnopengl.com/Guest-Articles/2020/Skeletal-Animation
     */
    [[nodiscard]] Triangle toViewSpaceSkinned(const SkinnedTriangle& model_space, const std::vector<glm::mat4>& bones) const {
        //deform triangle
        Triangle deformed_triangle = model_space.triangle;
        for (int i = 0; i < 3; ++i) {
            glm::vec4 vertex_position = {0,0,0,0};
            float remaining_weight = 1.0;
            for (int j = 0; j < MAX_BONES_PER_VERTEX; ++j) {
                int bone_id =  model_space.bone_ids[i][j];
                if(bone_id == -1) continue;
                vertex_position += (bones[bone_id] * deformed_triangle.pos[i]) * model_space.weights[i][j];
                remaining_weight -= model_space.weights[i][j];
                //todo support for normals
            }
            vertex_position += deformed_triangle.pos[i] * remaining_weight;
            deformed_triangle.pos[i] = vertex_position;
        }
        return toViewSpace(deformed_triangle); //do final projection
    }
};
