//
// Created by Philip on 7/18/2023.
//

#pragma once

#include <string>
#include "../Renderer/Texture.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
/**
 * Load a texture using stb image.
 * @param filepath Location of texture. jpg,png,tiff,or bmp.
 * @return Loaded texture.
 */
Texture loadTexture(const std::string& filepath){
    int width,height,channels;
    stbi_uc* image = stbi_load(filepath.c_str(),&width,&height,&channels, 0);
    Texture texture(width,height,channels);
    for (int x = 0; x < width; ++x) {
        for (int y = 0; y < height; ++y) {
            texture.setPixel(image[(y*width+x)*channels + 0],image[(y*width+x)*channels + 1],image[(y*width+x)*channels  + 2],x,y);
            if(channels == 4){
                texture.setTransparent(image[(y*width+x)*channels + 3], x,y);
            }
        }
    }
    stbi_image_free(image);
    return texture;
}
