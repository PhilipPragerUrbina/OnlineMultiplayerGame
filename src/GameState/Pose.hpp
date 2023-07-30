//
// Created by Philip on 7/19/2023.
//

#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

/**
 * Stores animation data, as well as skeleton data
 */
class Pose {

public:
    /**
    * Represents the transform of a bone at a single point in time
    */
    struct KeyFrame {
        glm::vec3 position;
        glm::quat rotation;
        glm::vec3 scale;
    };

    /**
    * Represents the animation of a single bone over time
    */
    struct BoneAnimation {
        std::vector<KeyFrame> bone_animation;
    };

    /**
     * Information about a single bone
     */
    struct Bone {
        std::vector<int> children; //The hierarchy
        glm::mat4 default_transform; //How it starts(local)
        glm::mat4 current_transform; //Local transform used for the current operation
        BoneAnimation animation; //What happens over time
        uint32_t element_id; //for fbx
    };

private:

    /**
     * Update the transforms of the bones recursively
     * @param global_transforms Transforms to write to
     * @param current_bone  Current bone to process
     * @param index Index of bone
     * @param parent_transform Last global transform
     */
    void updateRecursive(std::vector<glm::mat4>& global_transforms,const Bone& current_bone, size_t index, const glm::mat4& parent_transform) const {
        glm::mat4 global_transform = parent_transform * current_bone.current_transform;
        global_transforms[index] = global_transform;
        for (int child : current_bone.children) {
            updateRecursive(global_transforms, bones[child], child, global_transform);
        }
    }
public:

    /**
     * Use this to edit the pose
     * Order is specific to a skinned mesh
     * Last bone is root bone
     */
    std::vector<Bone> bones;

    /**
    * Compute the global transforms based on the current local transforms
    * @return Current global transforms of the bones in order
    */
    [[nodiscard]] std::vector<glm::mat4> computeGlobalTransforms() const {
        std::vector<glm::mat4> global_transforms(bones.size());
        assert(!bones.empty());
        Bone root = bones[bones.size()-1];
        updateRecursive(global_transforms,root,bones.size()-1,glm::identity<glm::mat4>());
        return global_transforms;
    }

    /**
     * Create a new pose
     * @param bones Pose data
     */
    Pose(const std::vector<Bone>& bones) : bones(bones) {}

    /**
     * Set the current transforms of the bones to the default transform
     */
    void setDefaultPose(){
        for (Bone& bone : bones) {
            bone.current_transform = bone.default_transform;
        }
    }

};
