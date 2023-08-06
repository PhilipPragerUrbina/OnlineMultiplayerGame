//
// Created by Philip on 8/5/2023.
//

#pragma once

#include <vector>
#include <unordered_map>
#include "../Renderer/Texture.hpp"
#include "../Renderer/Mesh.hpp"
#include "../Physics/PhysicsMesh.hpp"
#include "FBXLoader.hpp"
#include "OBJLoader.hpp"
#include "TextureLoader.hpp"

/**
 * Manages any read only shared resources from the drive.
 */
class ResourceManager {
public:
    /**
     * Use this to keep track if a resource
     */
    typedef uint16_t ResourceID;
private:
    std::vector<Texture> textures;
    std::vector<Mesh> meshes;
    std::vector<SkinnedMesh> skinned_meshes;
    std::vector<PhysicsMesh> physics_meshes;

    std::unordered_map<std::string,ResourceID> files; //Keep track of filenames associated to resources.

    ResourceID latest_available_id = 0;
public:
    /**
     * Supported 3d file formats
     */
    enum MeshFormat {
        FBX,OBJ
    };

    /**
     * Get a mesh resource from a filename.
     * @throws runtime_error Unable to load mesh.
     * @param filename File location of mesh.
     * @param format File format.
     * @return Mesh resource ID.
     */
    ResourceID getMesh(const std::string& filename, MeshFormat format){
        if(files.find(filename) == files.end()){ //Resource isn't yet created, load it.
            //todo create factory that detects extension and loader dependency injection
            //todo proper error handling in loader
            //todo proper material system, remove texture id from loader documentation.
            Mesh mesh{};
            switch (format) {
                case OBJ:
                    mesh = loadOBJ(filename);
                    break;
                case FBX:
                    mesh = loadFBX(filename);
                    break;
            }
            if(mesh.tris.empty()){
                throw std::runtime_error("Error loading mesh: " + filename);
            }
            meshes.push_back(mesh);
            files[filename] = latest_available_id;
            latest_available_id++;
        }
        return files[filename];
    }

    /**
    * Get a skinned mesh resource from a filename.
    * @throws runtime_error Unable to load mesh.
    * @param filename File location of mesh.
     * Currently only supports FBX.
    * @return Skinned Mesh resource ID.
     * @warning Normal mesh and skinned mesh can not be loaded from the same file at once. Todo fix.
    */
    ResourceID getSkinnedMesh(const std::string& filename){
        if(files.find(filename) == files.end()){ //Resource isn't yet created, load it.
            SkinnedMesh mesh = loadFBXSkinned(filename);
            if(mesh.tris.empty()){
                throw std::runtime_error("Error loading skinned mesh: " + filename);
            }
            skinned_meshes.push_back(mesh);
            files[filename] = latest_available_id;
            latest_available_id++;
        }
        return files[filename];
    }

    /**
     * Get a texture resource from a filename.
     * @throws runtime_error Unable to load texture.
     * @param filename File location of texture. JPG,JPEG,PNG,TIFF,or BMP.
     * @return Texture resource ID.
     */
    ResourceID getTexture(const std::string& filename){
        //todo file extension and access verification
        if(files.find(filename) == files.end()){ //Resource isn't yet created, load it.
            textures.push_back(loadTexture(filename));
            files[filename] = latest_available_id;
            latest_available_id++;
        }
        return files[filename];
    }

    /**
     * Generate a physics mesh from a normal mesh.
     * @param mesh Mesh resource id.
     * @return Physics mesh resource id.
     */
    ResourceID getPhysicsMesh(ResourceID mesh){
        //todo assert valid resource id.
        //todo find more elegant solution than generated filename
        //Generate "filename" identifier
        std::string filename = std::to_string(mesh) + "_collision_mesh";
        if(files.find(filename) == files.end()){ //Resource isn't yet created, load it.
            physics_meshes.emplace_back(meshes[mesh]);
            files[filename] = latest_available_id;
            latest_available_id++;
        }
        return files[filename];
    }

    /**
     * Access a mesh based on a resource id.
     * @param id Resource id.
     * @return Reference to mesh for reading.
     * @warning The returned reference is temporary, and will be invalidated by any non const methods on the resource manager.
     */
    const Mesh& readMesh(ResourceID id) const {
        return meshes[id];
    }

    /**
     * Access a skinned mesh based on a resource id.
     * @param id Resource id.
     * @return Reference to mesh for reading.
     * @warning The returned reference is temporary, and will be invalidated by any non const methods on the resource manager.
     */
    const SkinnedMesh& readSkinnedMesh(ResourceID id) const {
        return skinned_meshes[id];
    }

    /**
     * Access a physics mesh based on a resource id.
     * @param id Resource id.
     * @return Reference to physics mesh for reading.
     * @warning The returned reference is temporary, and will be invalidated by any non const methods on the resource manager.
     */
    const PhysicsMesh& readPhysicsMesh(ResourceID id) const {
        return physics_meshes[id];
    }

    /**
    * Access a texture based on a resource id.
    * @param id Resource id.
    * @return Reference to texture for reading.
    * @warning The returned reference is temporary, and will be invalidated by any non const methods on the resource manager.
    */
    const Texture& readTexture(ResourceID id) const {
        return textures[id];
    }

};
