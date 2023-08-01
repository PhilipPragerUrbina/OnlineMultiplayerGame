//
// Created by Philip on 7/18/2023.
//

#pragma once
#include <glm/glm.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtc/matrix_transform.hpp>
/**
 * This represents a camera that is used for projection
 */
class Camera {
private:
    glm::vec3 position;
    glm::vec3 look_at;
    glm::vec3 up;

    glm::mat4 transform;
    glm::mat4 projection;

    float near_plane, far_plane;

    /**
     * Update the transform matrix
     */
    void updateTransform(){
        transform = glm::lookAt(position,look_at, up);
    }
public:
    /**
     * Create a new perspective camera
     * @param fov_degrees Field of view in degrees
     * @param up Up vector
     * @param aspect_ratio Aspect ratio
     * @param near_plane Near projection plane
     * @param far_plane Far projection plane
     */
    Camera(float fov_degrees, const glm::vec3& up, float aspect_ratio,float near_plane = 0.1, float far_plane = 1000)
    : position({0,0,0}), look_at({0,0,0}), projection(glm::perspective(glm::radians(fov_degrees),aspect_ratio,near_plane,far_plane)), up(up), transform(), near_plane(near_plane),far_plane(far_plane) {
        updateTransform();
    }

    /**
     * Rotate the camera
     * @param rotation Amount to rotate by in euler angles
     */
    void rotate(const glm::vec3& rotation){
        glm::vec3 current_point = getLookAt();
        current_point -= getPosition();
        current_point =  current_point * glm::orientate3(rotation);
        current_point += getPosition();
        setLookAt(current_point);
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
     * Get near clipping plane
     */
    [[nodiscard]] float getNearPlane() const {
        return near_plane;
    }

    /**
    * Get far clipping plane
    */
    [[nodiscard]] float getFarPlane() const {
        return far_plane;
    }


};
