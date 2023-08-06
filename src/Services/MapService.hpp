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
public:

    void registerMap(  uint16_t collider){
        map = collider;
    }

    void deRegisterMap( ){
        //unusded
    }

    [[nodiscard]]  uint16_t queryCollider() const {
        return map;
    }


};
