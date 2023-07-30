//
// Created by Philip on 7/19/2023.
//

#pragma once
#include <string>
#include <iostream>
#include "../Renderer/SkinnedMesh.hpp"
#include "ufbx/ufbx.h"
#include "../Renderer/Mesh.hpp"
#include <algorithm>
#include <glm/gtc/type_ptr.hpp>
#include "glm/gtc/quaternion.hpp"

/**
 * Recurse bone nodes of fbx scene
 * @param scene Scene
 * @param animation_data Where to put bones
 * @param node_id Node typed id
 * @return ID of new bone in animation data vector
 */
size_t recurseBones(ufbx_scene* scene, std::vector<Pose::Bone> & animation_data, uint32_t node_id){
    ufbx_node* fbx_bone = scene->nodes[node_id];
    Pose::Bone bone{};
    bone.element_id = fbx_bone->element_id;
    bone.default_transform =glm::identity<glm::mat4>();
    for (auto node : fbx_bone->children) {
        bone.children.push_back((int)recurseBones(scene,animation_data,node->typed_id));
    }
    animation_data.push_back(bone);
    return animation_data.size()-1;
}



/**
 * Load a FBX file using UFBX
 * Loads it all into one mesh
 * If you want Animations or skinning, use loadFBXSkinned()
 * @param filepath Location of FBX
 * @param texture_id Texture id to assign to mesh
 * Returns empty mesh on failure
 * @return Mesh
 */
Mesh loadFBX(const std::string& filepath, int texture_id){
    Mesh output_mesh{};

    ufbx_error error;
    ufbx_scene *scene = ufbx_load_file(filepath.c_str(), nullptr, &error);
    if (!scene) {
        std::cerr << "UFBX failed to load: " << error.description.data << "\n";
        return output_mesh;
    }

    //Collect mesh data
    for (size_t  i = 0; i < scene->meshes.count; ++i) {
        ufbx_mesh* mesh = scene->meshes.data[i];
        for (size_t j = 0; j < mesh->faces.count; ++j) {
            ufbx_face face = mesh->faces.data[j];
            //Triangulate
            size_t indices_count = ufbx_get_triangulate_face_num_indices(face);
            auto* tri_indices = new uint32_t[indices_count];
            ufbx_triangulate_face(tri_indices,indices_count,mesh,face);
            //for each triangle
            for (size_t k = 0; k < indices_count / 3; ++k) {
                Triangle triangle{};
                triangle.texture_id = texture_id;
                //for each vertex
                for (int v = 0; v < 3; ++v) {
                    auto index = tri_indices[k*3+v];
                    triangle.pos[v] = {mesh->vertex_position[index].x, mesh->vertex_position[index].y, mesh->vertex_position[index].z, 1.0};
                    if(mesh->vertex_normal.exists){
                        triangle.norm[v] = {mesh->vertex_normal[index].x, mesh->vertex_normal[index].y, mesh->vertex_normal[index].z};
                    }else{
                        triangle.norm[v] = {0,0,0};
                        //todo auto generate normals
                    }
                    if(mesh->vertex_uv.exists){
                        triangle.tex[v] = {mesh->vertex_uv[index].x, 1.0-mesh->vertex_uv[index].y}; //flip
                    }else{
                        std::cout << "UBFX Warning: " << "Missing UVs" << "\n";
                        triangle.tex[v] = {0,0};
                    }
                }
                output_mesh.tris.push_back(triangle);
            }
            delete[] tri_indices;
        }
    }
    ufbx_free_scene(scene);
    return output_mesh;
}


/**
 * Load a FBX file using UFBX
 * Only loads one mesh
 * Only use this for animated meshes. Make sure that there is one main root node that all other bones are directly or indirectly parented to. If there are multiple roots, some bones may be missing.
 * @param filepath Location of FBX
 * @param texture_id Texture id to assign to mesh
 * @return Skinned mesh with default pose
 * Currently only loads skeleton,animation is not yet supported
 * Returns empty mesh on failure
 */
SkinnedMesh loadFBXSkinned(const std::string& filepath, int texture_id){
    SkinnedMesh output_mesh{};

    ufbx_error error;
    ufbx_scene *scene = ufbx_load_file(filepath.c_str(), nullptr, &error);
    if (!scene) {
        std::cerr << "UFBX failed to load: " << error.description.data << "\n";
        return output_mesh;
    }

    std::vector<Pose::Bone> default_animation;
    //Get bone data
    if(scene->skin_clusters.count > 0){
        recurseBones(scene, default_animation, scene->skin_clusters[0]->bone_node->typed_id);

        output_mesh.num_bones = (int)default_animation.size();
        output_mesh.animations.emplace_back(default_animation);
    }

    //Load animations
    //todo finish animation loader
    /*
    for (int i = 0; i < scene->anim_stacks.count; ++i) {
        std::vector<Animation::Bone> animation = default_animation; //copy bone layout
        auto animation_stack = scene->anim_stacks[i];
        float duration = animation_stack->time_end - animation_stack->time_begin;
        if(animation_stack->layers.count == 0) continue;
        auto animation_layer = animation_stack->layers[0];
        for (int j = 0; j < animation_layer->anim_props.count; ++j) {
            auto bone_animation = animation_layer->anim_props[j];
            auto element_id = bone_animation.element->element_id;
            auto property = bone_animation.prop_name;
            auto keyframes_x = bone_animation.anim_value->curves[0]->keyframes;
            auto keyframes_y = bone_animation.anim_value->curves[1]->keyframes;
            auto keyframes_z = bone_animation.anim_value->curves[2]->keyframes;
            if(std::string(property.data) == "LclTranslation "){
                for (int l = 0; l < keyframes_x.count; ++l) {
                    bone_animation.
                }
            }
        }


        output_mesh.animations.emplace_back(duration,animation);

    }*/

    //Collect mesh data
    for (size_t  i = 0; i < scene->meshes.count; ++i) {
        ufbx_mesh* mesh = scene->meshes.data[i];
        for (size_t j = 0; j < mesh->faces.count; ++j) {
            ufbx_face face = mesh->faces.data[j];
            //Triangulate
            size_t indices_count = ufbx_get_triangulate_face_num_indices(face);
            auto* tri_indices = new uint32_t[indices_count];
            ufbx_triangulate_face(tri_indices,indices_count,mesh,face);
            //for each triangle
            for (size_t k = 0; k < indices_count / 3; ++k) {
                Triangle triangle{};
                SkinnedTriangle skinned_triangle{};
                triangle.texture_id = texture_id;
                //for each vertex
                for (int v = 0; v < 3; ++v) {
                    auto index = tri_indices[k*3+v];
                    triangle.pos[v] = {mesh->vertex_position[index].x, mesh->vertex_position[index].y, mesh->vertex_position[index].z, 1.0};
                    if(mesh->vertex_normal.exists){
                        triangle.norm[v] = {mesh->vertex_normal[index].x, mesh->vertex_normal[index].y, mesh->vertex_normal[index].z};
                    }else{
                        triangle.norm[v] = {0,0,0};
                    }
                    if(mesh->vertex_uv.exists){
                        triangle.tex[v] = {mesh->vertex_uv[index].x, 1.0-mesh->vertex_uv[index].y};
                    }else{
                        std::cout << "UBFX Warning: " << "Missing UVs" << "\n";
                        triangle.tex[v] = {0,0};
                    }

                    //Reset weights
                    for (int l = 0; l < MAX_BONES_PER_VERTEX; ++l) {
                        skinned_triangle.bone_ids[v][l] = -1;
                    }


                    for (size_t l = 0; l < mesh->skin_deformers.count; ++l) {
                        ufbx_skin_deformer* bone = mesh->skin_deformers[l];
                        auto skin_vertex = bone->vertices[mesh->vertex_position.indices[index]];
                        for (size_t h = 0; h < skin_vertex.num_weights; ++h) {
                            auto wieght_index = skin_vertex.weight_begin + h;
                            auto weight = bone->weights[wieght_index];
                            for (int m = 0; m < MAX_BONES_PER_VERTEX; ++m) { //Find open bone or less important bone to replace in the vertex
                                if(skinned_triangle.bone_ids[v][m] == -1 || skinned_triangle.weights[v][m] < weight.weight){
                                    uint32_t element_id = bone->clusters[weight.cluster_index]->bone_node->element_id;
                                    if(std::find_if(default_animation.begin(), default_animation.end(), [&element_id](const Pose::Bone& other_bone){return other_bone.element_id == element_id;}) != default_animation.end()){
                                        skinned_triangle.bone_ids[v][m] = (int)(std::find_if(default_animation.begin(), default_animation.end(), [&element_id](const Pose::Bone& other_bone){return other_bone.element_id == element_id;}) - default_animation.begin());
                                    }else{
                                        skinned_triangle.bone_ids[v][m] = -1;
                                    }
                                    skinned_triangle.weights[v][m] = (float)weight.weight;
                                    break;
                                }
                            }
                        }
                    }
                }
                skinned_triangle.triangle = triangle;
                output_mesh.tris.push_back(skinned_triangle);
            }
            delete[] tri_indices;
        }
    }
    ufbx_free_scene(scene);
    return output_mesh;
}

