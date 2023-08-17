//
// Created by Philip on 8/11/2023.
//

#pragma once

#include "GameObject.hpp"
#include <glm/gtx/quaternion.hpp>

/**
 * Car settings
 */
struct CarParams {

};

/**
 * Physical state of the car for prediction
 */
struct CarState {
    glm::vec3 position;
    glm::quat rotation;
    glm::vec3 velocity;
    glm::vec3 angular_velocity;
};

//todo fix car values
//todo add direct x renderer
//todo add cubmap generator and light list
//todo have option to render in update thread
//todo add fixed update for physics,with the option to overflow into dynamic update if needed

/**
 * Simulate a vehicle
 */
class Car : public GameObjectImpl<Car,CarParams,CarState>{
private:
    //state the car starts in and resets to
    constexpr static const CarState start_state{
            {0,0,1.5},glm::identity<glm::quat>(),{0,0,0}, {0,0,0}
    };

    //state shared between client and server
    CarState shared_state =start_state;

    //Forces
    glm::vec3 net_torque = {0,0,0};
    glm::vec3 net_force = {0,0,0};

    //Directions
    constexpr static glm::vec3 upward = {0,0,1};
    constexpr static glm::vec3 forward = {0,1,0};
    constexpr static glm::vec3 side = {1,0,0};

    //Inertia and mass
    constexpr static const glm::vec3 car_dimensions = {1,2,0.7}; //dimensions of main car box

    constexpr static float mass = 1000;
    //intertia tensor of a cuboid
    constexpr static glm::mat3 inertia_tensor = {
            (1.0f/12.0f)*mass * (car_dimensions.y*car_dimensions.y+car_dimensions.z*car_dimensions.z),0,0,
            0,(1.0f/12.0f)*mass * (car_dimensions.x*car_dimensions.x+car_dimensions.z*car_dimensions.z),0,
            0,0,(1.0f/12.0f)*mass * (car_dimensions.x*car_dimensions.x+car_dimensions.y*car_dimensions.y)
    };

    //transforms computed on update
    glm::mat4 body_transform = glm::identity<glm::mat4>();
    glm::mat4 body_rotation = glm::identity<glm::mat3>();
    glm::mat4 body_rotation_inverse = glm::identity<glm::mat3>();

    //get a global point from a local point
    glm::vec3 toGlobal(const glm::vec3& local_position) const {
        return body_transform * glm::vec4(local_position,1.0f);
    }

    //get a global direction from a local direction
    glm::vec3 toGlobalRotation(const glm::vec3& local_rotation) const {
        return body_rotation * glm::vec4(local_rotation,1.0f);
    }

    //Raycast to the terrain using local coordinates. Returns negative values if not hit.
    [[nodiscard]] float raycastTerrain(const glm::vec3& direction_local,const glm::vec3& origin_local,const PhysicsMesh* map) const{
        float distance = -1.0;
        if(map->rayCast(toGlobal(origin_local), toGlobalRotation(direction_local),distance)){
            return distance;
        }
        return -1.0;
    }

    //Raycast to the ground plane using local coordinates. Returns negative values if not hit.
    [[nodiscard]] float raycastGroundPlane(const glm::vec3& direction_local,const glm::vec3& origin_local) const {
        glm::vec3 origin = toGlobal(origin_local);
        glm::vec3 direction = toGlobalRotation(direction_local);
        float dot_product = glm::dot(direction,-upward);
        if(dot_product > 0.00000001 && origin.z > 0){ //facing towards and above
            return (-origin.z)/direction.z; //Compute distance with simple solved equation
        }
        return-1.0f;
    }

    /**
     * Apply a force to a point on the vehicle for this time step.
     * @param position Position of force applied in world space.
     * @param force Magnitude and direction of force in world space.
     */
    void applyForce(const glm::vec3& position, const glm::vec3& force){
        net_force += force;
        net_torque += glm::cross(position - shared_state.position, force); //Center of gravity is the origin
    }

    /**
     * Get the local velocity at a local point
     */
    glm::vec3 getLocalVelocityAtPoint(const glm::vec3& local_point){
        glm::vec3 local_angular_velocity = body_rotation_inverse * glm::vec4(shared_state.angular_velocity, 1);
        glm::vec3 local_velocity = body_rotation_inverse * glm::vec4(shared_state.velocity, 1);
        return local_velocity + glm::cross(local_angular_velocity,local_point);
    }

    /**
    * Apply a force to a point on the vehicle for this time step.
    * @param position Position of force applied in local space.
    * @param force Magnitude and direction of force in local space.
    */
    void applyForceLocal(const glm::vec3& position, const glm::vec3& force){
        applyForce(toGlobal(position), toGlobalRotation(force) );
    }
    /**
     * Update the body after force has been applied
     * @param delta_time
     */
    void updateBody(float delta_time){

        shared_state.velocity += delta_time * (net_force / mass);
        shared_state.position += delta_time * shared_state.velocity;

        shared_state.angular_velocity += delta_time * (glm::inverse(inertia_tensor) * (net_torque - glm::cross(shared_state.angular_velocity, inertia_tensor * shared_state.angular_velocity)));
        shared_state.rotation += delta_time * (0.5f * glm::quat{0, shared_state.angular_velocity.x, shared_state.angular_velocity.y, shared_state.angular_velocity.z} * shared_state.rotation);
        shared_state.rotation = glm::normalize(shared_state.rotation);

        net_torque = {0,0,0};
        net_force = {0,0,0};

        body_rotation = glm::mat4_cast(shared_state.rotation);
        body_transform = glm::translate(glm::identity<glm::mat4>(),shared_state.position) * body_rotation;
        body_rotation_inverse = glm::inverse(body_rotation);
    }

    struct Wheel {
        const glm::vec3 local_anchor;
        const bool flip;
        glm::vec3 local_position = local_anchor;
        float angle = 0;
        float spin = 0;
    };

    Wheel wheels[4] = {Wheel{glm::vec3{0.45,0.78,-0.37}, false},Wheel{ glm::vec3{0.45,-0.65,-0.37}, false},Wheel{glm::vec3{-0.45,0.78,-0.37},true},Wheel{glm::vec3{-0.45,-0.65,-0.37},true} };


    ResourceManager::ResourceID mesh_main;
    ResourceManager::ResourceID mesh_wheel;

    ResourceManager::ResourceID test_texture;

    bool player = false;

    Camera camera{90, {0,0,-1},1};

public:

    void loadResourcesClient(ResourceManager &manager, bool associated) override {
        player = associated;
        mesh_main = manager.getMesh("vehicle_game/car.obj",ResourceManager::OBJ);
        mesh_wheel = manager.getMesh("vehicle_game/wheel.obj",ResourceManager::OBJ);
        test_texture = manager.getTexture("test_textures/test.png");
    }

    void loadResourcesServer(ResourceManager &manager) override {
        //nothing
    }

    void registerServices(Services &services) override {
        //nothing
    }

    void deRegisterServices(Services &services) override {
        //nothing
    }

    void update(int delta_time_ms, const EventList &events, const Services &services,const ResourceManager &resource_manager) override {

        std::this_thread::sleep_for(std::chrono::milliseconds(1)); //Apply delay so delta time has something

        auto delta_time = (float)delta_time_ms;

        if(events.keys[5]){ //reset
            shared_state = start_state;
        }
        if(events.keys[6]){ //reset in place
            glm::vec3 last_position = shared_state.position;
            shared_state = start_state;
            shared_state.position = {last_position.x,last_position.y,shared_state.position.z}; //Preserve current xy position
        }

        net_force = {0,0,-0.004f}; //gravity

        //suspension force
        const float rest_distance = 0.1f;
        const float wheel_radius = 0.376f /2.0f;
        const float strength = 0.04f;
        const float damping = 0.4f;
        const float steering_angle = glm::radians(25.0f); //radians
        const float top_speed = 1.0f;
        const float break_force = 0.1f;
        const float drift_resistance = 0.4f;
        const float acceleration =  0.0003f;

        //todo delta time to seconds and these to realistic values in m/s^2 ans such

        for (int i = 0; i < 4; ++i) {
            float distance_to_floor = services.map_service.hasCollider() ? raycastTerrain(-upward,wheels[i].local_anchor,resource_manager.readPhysicsMesh(services.map_service.queryCollider())) : raycastGroundPlane(-upward,wheels[i].local_anchor);


            if(distance_to_floor > 0.0 && distance_to_floor < rest_distance+wheel_radius){

                float offset = rest_distance - (distance_to_floor-wheel_radius);
                glm::vec3 velocity = getLocalVelocityAtPoint(wheels[i].local_anchor);
                float force = (offset*strength)-(velocity.z*damping); //todo test exponential

                applyForceLocal(wheels[i].local_anchor,upward*force);

                wheels[i].local_position = wheels[i].local_anchor - glm::vec3 {0,0,distance_to_floor-wheel_radius};

                //todo check what things to multiply by delta time

                //steering
                //todo have things work in the air
                wheels[i].angle = 0;
                glm::vec3 thrust_direction = forward;
                if(events.keys[2] && (i == 0 || i == 2)) {
                    thrust_direction = glm::rotate(thrust_direction ,-steering_angle,upward);
                    wheels[i].angle = -steering_angle;
                }
                if(events.keys[3]  && (i == 0 || i == 2)) {
                    thrust_direction = glm::rotate(thrust_direction ,steering_angle,upward);
                    wheels[i].angle = steering_angle;
                }

                const float circumference = 2.0f * glm::pi<float>() * wheel_radius;
                wheels[i].spin -= ((glm::dot(velocity,thrust_direction) * delta_time) / circumference) * (2.0f * glm::pi<float>());

                //throttle
                float speed =acceleration;
                if(velocity.y > top_speed){
                    speed = 0;
                }
                if(events.keys[0]){
                    applyForceLocal( wheels[i].local_position,thrust_direction*speed);
                }

                //todo switch to dot
                //break force
                if(events.keys[1] && velocity.y > acceleration){
                    float forward_velocity = glm::dot(velocity,thrust_direction);
                    applyForceLocal( wheels[i].local_position, -thrust_direction * forward_velocity * break_force);
                } else if(events.keys[1]) { //reverse
                    applyForceLocal( wheels[i].local_position,-thrust_direction*speed);
                }

                //slide

                float wrong_side_velocity = glm::dot(velocity,{-thrust_direction.y,thrust_direction.x,thrust_direction.z});
                applyForceLocal(wheels[i].local_position, -glm::vec3 {-thrust_direction.y,thrust_direction.x,thrust_direction.z} * wrong_side_velocity * drift_resistance);

            }

        }

        updateBody(delta_time);
    }



    void predict(int delta_time, const EventList &events, const Services &services,
                 const ResourceManager &resource_manager) override {

        const float SENSITIVITY = 100.0f;
        const float SCROLL_SENSITIVITY = 10.0f;
        const float DISTANCE_CLAMP_MIN = 1.5f;

        float camera_distance = ((float)events.mouse_scroll / -SCROLL_SENSITIVITY) + DISTANCE_CLAMP_MIN; //Make sure the scroll starts in a usable state

        if(camera_distance < DISTANCE_CLAMP_MIN){
            camera_distance = DISTANCE_CLAMP_MIN;
        }

        float current_radians_x =  (float)events.mouse_x / -SENSITIVITY;
        float current_radians_y = (float)events.mouse_y / SENSITIVITY;
        glm::vec3 camera_position;
        camera_position.x = sinf(current_radians_x);
        camera_position.y = cosf(current_radians_x);
        camera_position.z = cosf(current_radians_y) * 2.0f;

        camera.setPosition(body_transform * glm::vec4 (camera_position * camera_distance, 1));
        camera.setLookAt(shared_state.position);

        update(delta_time,events,services,resource_manager);
    }

    void updateServices(Services &services) const override {
        //nothing
    }

    bool updateCamera(glm::vec3 &position, glm::vec3 &look_at) const override{
        if(player){
            position = camera.getPosition();
            look_at = camera.getLookAt();
            return true;
        }
        return false;
    }

    void render(Renderer &renderer, const ResourceManager &resource_manager) const override {
        for (int i = 0; i < 4; ++i) {
            if(wheels[i].flip){
                renderer.queueDraw(resource_manager.readMesh(mesh_wheel),body_transform * glm::translate(glm::identity<glm::mat4>(), wheels[i].local_position)  * glm::rotate(glm::identity<glm::mat4>(),wheels[i].angle, upward) * glm::rotate(glm::identity<glm::mat4>(), wheels[i].spin , side) * glm::scale(glm::identity<glm::mat4>(), {-1, 1, 1}), resource_manager.readTexture(test_texture));
            }else{
                renderer.queueDraw(resource_manager.readMesh(mesh_wheel), body_transform * glm::translate(glm::identity<glm::mat4>(), wheels[i].local_position) * glm::rotate(glm::identity<glm::mat4>(),wheels[i].angle, upward)  *  glm::rotate(glm::identity<glm::mat4>(), wheels[i].spin , side) , resource_manager.readTexture(test_texture));
            }

        }
        renderer.queueDraw(resource_manager.readMesh(mesh_main), body_transform, resource_manager.readTexture(test_texture));
    }

    SphereBV getBounds() const override {
        //todo server culling
        //todo indices and map culling, also clean up physics mesh
        //todo partial transparency with blending buffer
        //todo cube maps
        return SphereBV{shared_state.position,2.0f};
    }

protected:
    [[nodiscard]] CarState serializeInternal() const override {
        return shared_state;
    }

    void deserializeInternal(const CarState &state) override {
        shared_state = state;
    }

    [[nodiscard]] std::unique_ptr<GameObject> createNewInternal(const CarParams &params) const override {
        return std::unique_ptr<GameObject>(new Car());
    }

    [[nodiscard]] CarParams getConstructorParamsInternal() const override {
        return CarParams();
    }

};
