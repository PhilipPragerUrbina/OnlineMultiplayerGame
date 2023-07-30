//
// Created by Philip on 7/18/2023.
//

#pragma once

#include <cstdint>
#include <vector>
#include <cassert>
#include <iostream>

/**
 * A simple 8-bit color texture
 */
class Texture {
private:
    std::vector<uint8_t> pixels;
    const int channels;
    const int width,height;
public:
    /**
     * Create an empty texture
     * @param width Width in pixels
     * @param height Height in pixels
     * @param channels Channel count. At least 3(r,g,b).
     */
    Texture(int width,int height,int channels = 3) : channels(channels), width(width), height(height), pixels(channels*width*height){
        assert(channels>=3);
    }


    /**
     * Check if a pixel is transparent
     * @param x,y Coordinate. Must be in bounds.
     * Will return false if no alpha channel
     * @return True if transparent
     */
    [[nodiscard]] bool isTransparent(int x,int y) const {
        assert(x >= 0 && x < width);
        assert(y >= 0 && y < height);
        if(channels < 4) return false;
        int offset = (y*width+x)*channels;
        return pixels[offset+3] < 100;
    }

    /**
     * Set the transparency of a pixel
     * @param alpha Transparency value. Less than 100 is considered transparent.
     * @param x,y Coordinate. Must be in bounds.
     * Must have more than 3 channels
     */
    void setTransparent(uint8_t alpha, int x,int y){
        assert(x >= 0 && x < width);
        assert(y >= 0 && y < height);
        assert(channels > 3);
        int offset = (y*width+x)*channels;
        pixels[offset+3]= alpha;
    }

    /**
     * Set r g b pixel
     * @param r Red
     * @param g Green
     * @param b Blue
     * @param x,y Coordinate. Must be in bounds.
     */
    void setPixel(uint8_t r,uint8_t g,uint8_t b, int x, int y){
        assert(x >= 0 && x < width);
        assert(y >= 0 && y < height);
        int offset = (y*width+x)*channels;
        pixels[offset + 0] = r;
        pixels[offset + 1] = g;
        pixels[offset + 2] = b;
    }

    /**
     * Simple 8-bit color struct
     */
    struct Color {
        uint8_t r,g,b;
    };

    /**
     * Get color of pixel
     * @param x,y Coordinates. Must be in bounds.
     * @return RGB color
     */
    [[nodiscard]] Color getPixel(int x,int y) const {
        assert(x >= 0 && x < width);
        assert(y >= 0 && y < height);
        int offset = (y*width+x)*channels;
        return Color{ pixels[offset + 0], pixels[offset + 1], pixels[offset + 2]};
    }

    /**
     * Get width in pixels
     */
    [[nodiscard]] int getWidth() const{
        return width;
    }

    /**
     * Get height in pixels
     */
    [[nodiscard]] int getHeight() const{
        return height;
    }
};
