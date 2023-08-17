//
// Created by Philip on 7/18/2023.
//

#pragma once
#include <glm/glm.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtc/matrix_transform.hpp>

/**
 * A plane in 3D space
 */
struct Plane {
    glm::vec3 offset; //Any point on the plane
    glm::vec3 normal; //The direction the plane is facing.
};

/**
 * This represents a perspective camera
 */
class Camera {
private:
    glm::vec3 position{2,2,2}; //Location of camera origin
    glm::vec3 look_at{0,0,0}; //Location camera is oriented towards
    glm::vec3 up; //Vector in the up direction

    glm::mat4 transform{}; //View matrix
    glm::mat4 projection; //Projection matrix

    float near_plane_distance, far_plane_distance; //Clipping plane locations

    std::array<Plane,6> planes_global{}; //all frustum planes

    float fov_radians, aspect_ratio;

    /**
     * Update the transform matrix and global planes
     * @see https://learnopengl.com/Guest-Articles/2021/Scene/Frustum-Culling
     */
    void updateTransform(){
        transform = glm::lookAt(position,look_at, up); //update transforms

        //update planes
        glm::vec3 facing_direction = glm::normalize(look_at - position);
        glm::vec3 right_direction =  glm::normalize(glm::cross(facing_direction, up));
        glm::vec3 up_direction = glm::normalize(glm::cross(right_direction, facing_direction));

        //Get dimensions of far plane
        float far_plane_half_width = far_plane_distance * tanf(fov_radians * 0.5f);
        float far_plane_half_height = far_plane_half_width * aspect_ratio;

        glm::vec3 far_plane_multiplier = far_plane_distance * facing_direction;

        planes_global[0] = { position + near_plane_distance * facing_direction, facing_direction }; //Near plane
        planes_global[1] = { position + far_plane_multiplier, -facing_direction }; //far plane

        planes_global[2] = { position, glm::cross(far_plane_multiplier - right_direction * far_plane_half_width, up_direction) }; //Right plane
        planes_global[3] = { position, glm::cross(up_direction , far_plane_multiplier + right_direction * far_plane_half_width) }; //Left plane

        planes_global[4] = { position, glm::cross(right_direction, far_plane_multiplier - up_direction * far_plane_half_height)  }; //Upper plane
        planes_global[5] = { position,glm::cross(far_plane_multiplier + up_direction * far_plane_half_height, right_direction) }; //Lower plane
    }

public:

    /**
     * Create a new perspective camera
     * @param fov_degrees Field of view in degrees
     * @param up Up vector
     * @param aspect_ratio Aspect ratio
     * @param near_plane_distance Near projection distance
     * @param far_plane_distance Far projection distance
     */
    Camera(float fov_degrees, const glm::vec3& up, float aspect_ratio,float near_plane_distance = 0.1f, float far_plane_distance = 1000.0f) :  up(up)
    , projection(glm::perspective(glm::radians(fov_degrees),aspect_ratio,near_plane_distance,far_plane_distance)), near_plane_distance(near_plane_distance), far_plane_distance(far_plane_distance), fov_radians(glm::radians(fov_degrees)), aspect_ratio(aspect_ratio) {
        updateTransform();
    }

    /**
     * Update the aspect ratio of the camera and recalculate projection matrix
     * @param new_aspect_ratio The new aspect ratio
     */
    void updateAspectRatio(float new_aspect_ratio){
        aspect_ratio = new_aspect_ratio;
        projection = glm::perspective(fov_radians,aspect_ratio,near_plane_distance,far_plane_distance);
    }

    /**
     * Get the projection matrix of the camera
     */
    [[nodiscard]] const glm::mat4& getProjection() const {
        return projection;
    }

    /**
     * Get the inverse combined projection and view matrix
     * Can be used to go from clip space to world space
     */
    [[nodiscard]] glm::mat4 getInverseMatrix() const {
        return glm::inverse(projection*transform);
    }

    /**
     * Get the transformation matrix of the camera
     */
    [[nodiscard]] const glm::mat4& getTransform() const {
        return transform;
    }

    /**
     * Get the position of the camera
     */
    [[nodiscard]] const glm::vec3& getPosition() const {
        return position;
    }

    /**
     * Get where the camera is looking
     */
    [[nodiscard]] const glm::vec3 &getLookAt() const {
        return look_at;
    }

    /**
     * Set the position of the camera
     */
    void setPosition(const glm::vec3 &new_position) {
        position = new_position;
        updateTransform();
    }

    /**
     * Set the look at target of the camera
     */
    void setLookAt(const glm::vec3 &new_look_at) {
        look_at = new_look_at;
        updateTransform();
    }

    /**
     * Get near clipping plane distance
     */
    [[nodiscard]] float getNearPlaneDistance() const {
        return near_plane_distance;
    }

    /**
    * Get far clipping plane distance
    */
    [[nodiscard]] float getFarPlaneDistance() const {
        return far_plane_distance;
    }

    /**
     * Get world space frustum planes
     */
    [[nodiscard]] const std::array<Plane,6>& getFrustumPlanes() const {
        return planes_global;
    }
};
