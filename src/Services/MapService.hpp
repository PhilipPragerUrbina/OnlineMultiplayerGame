//
// Created by Philip on 8/6/2023.
//

#pragma once




/**
 * Keeps track of the current map
 */
class MapService {
private:
     uint16_t map;
     bool init = false;
public:

    void registerMap(  uint16_t collider){
        map = collider;
        init = true;
    }

    void deRegisterMap( ){
        //unusded
    }

    [[nodiscard]]  uint16_t queryCollider() const {
        return map;
    }

    [[nodiscard]]  bool hasCollider() const {
        return init;
    }


     glm::vec3 chaser = {0,0,1};
};
