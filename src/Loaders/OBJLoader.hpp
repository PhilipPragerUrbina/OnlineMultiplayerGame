//
// Created by Philip on 7/18/2023.
//

#pragma once


#include <string>
#include <iostream>
#include "../Renderer/Mesh.hpp"
#define TINYOBJLOADER_IMPLEMENTATION
#include "tinyobj/tinyobjloader.h"

//todo multiple textures per mesh

/**
 * Load obj file using tinyOBJ
 * @param filepath Filepath to obj file
 * @param texture_id Texture ID to use for this mesh
 * @return Resulting mesh
 */
Mesh loadOBJ(const std::string& filepath){
    //This code is mostly based on the tiny objloader example

    tinyobj::ObjReaderConfig reader_config;
    reader_config.mtl_search_path = "./";
    tinyobj::ObjReader reader;
    Mesh mesh;

    if (!reader.ParseFromFile(filepath, reader_config)) {
        if (!reader.Error().empty()) {
            std::cerr << "TinyObjReader: " << reader.Error();
        }
        return mesh;
    }

    if (!reader.Warning().empty()) {
        std::cout << "TinyObjReader: " << reader.Warning();
    }

    auto& attrib = reader.GetAttrib();
    auto& shapes = reader.GetShapes();
    auto& materials = reader.GetMaterials();

// Loop over shapes
    for (const auto & shape : shapes) {
        // Loop over faces(polygon)
        size_t index_offset = 0;
        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
            auto fv = size_t(shape.mesh.num_face_vertices[f]);

            Triangle triangle{};
            if(fv == 3){
                // Loop over vertices in the face.
                for (size_t v = 0; v < 3; v++) {
                    // access to vertex
                    tinyobj::index_t idx = shape.mesh.indices[index_offset + v];
                    triangle.pos[v] = {attrib.vertices[3*size_t(idx.vertex_index)+0], attrib.vertices[3*size_t(idx.vertex_index)+1], attrib.vertices[3*size_t(idx.vertex_index)+2],1.0};

                    // Check if `normal_index` is zero or positive. negative = no normal data
                    if (idx.normal_index >= 0) {
                        triangle.norm[v] = {attrib.normals[3*size_t(idx.normal_index)+0],attrib.normals[3*size_t(idx.normal_index)+1],attrib.normals[3*size_t(idx.normal_index)+2] };
                    } else {
                        //todo auto generate normals
                    }

                    // Check if `texcoord_index` is zero or positive. Negative = no texcoord data
                    if (idx.texcoord_index >= 0) {
                        triangle.tex[v] = {attrib.texcoords[2*size_t(idx.texcoord_index)+0],1.0 - attrib.texcoords[2*size_t(idx.texcoord_index)+1]}; //flip
                    } else {
                        //todo auto generate tex coords
                        std::cout << "TinyObjReader: " << "Missing tex coords";
                    }
                }
            } else {
                std::cout << "TinyObjReader: " << "Non triangles in this mesh";
            }

            mesh.tris.push_back(triangle);
            index_offset += fv;

            // per-face material
            shape.mesh.material_ids[f];
        }
    }
    return mesh;
}