//
// Created by Philip on 7/18/2023.
//

#pragma once
#include "glm/glm.hpp"
/**
 * Shade pixels
 */
class FragmentShader {
private:
    //todo generalized version
public:
    //todo transparency
    /**
     * Run the fragment shader
     * @param position Pixel position
     * @param normal Pixel normal
     * @param uv Pixel uv
     * @param texture Texture RGB from triangle texture id and UV
     * @return RGB
     */
    static glm::vec3 run(const glm::vec3& position, const glm::vec3& normal, const glm::vec2& uv, const glm::vec3& texture){
        return texture;
    }
};
