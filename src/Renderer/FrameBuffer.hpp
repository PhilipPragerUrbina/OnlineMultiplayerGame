//
// Created by Philip on 7/18/2023.
//

#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <iostream>

/**
 * The final image is stored here
 */
class FrameBuffer {
private:
    std::vector<glm::vec4> pixels; // r,g,b,depth
    std::vector<uint8_t> bit_pixels; //rgba
    const int width,height;
public:

    /**
     * Create a new framebuffer
     * @param width Width in pixels
     * @param height Height in pixels
     * @param default_value Initial pixel values. R,G,B,Depth
     */
    FrameBuffer(int width, int height, const glm::vec4& default_value) : width(width), height(height), pixels(width*height), bit_pixels(width*height*4) {
        for(glm::vec4& pixel : pixels){
            pixel = default_value;
        }
    }

    /**
     * Get pixel value at coordinate
     * @param x,y Coordinate. Must be in bounds.
     * @return R,G,B,Depth
     */
    [[nodiscard]] const glm::vec4& getPixel(int x, int y) const {
        assert(x >= 0 && x < width);
        assert(y >= 0 && y < height);
        return pixels[y*width+x];
    }

    /**
    * Set pixel value at coordinate
    * @param x,y Coordinate. Must be in bounds.
    * @param new_pixel R,G,B,Depth
    */
    void setPixel(int x, int y, const glm::vec4& new_pixel) {
        assert(x >= 0 && x < width);
        assert(y >= 0 && y < height);
        pixels[y*width+x] = new_pixel;
        int offset = (y*width+x)*4;
        bit_pixels[offset + 0] = (uint8_t)new_pixel.x;
        bit_pixels[offset + 1] = (uint8_t)new_pixel.y;
        bit_pixels[offset + 2] = (uint8_t)new_pixel.z;
        bit_pixels[offset + 3] = 255;
    }

    /**
   * Set pixel value at coordinate if depth is less than current pixel
   * @param x,y Coordinate. Must be in bounds.
   * @param new_pixel R,G,B,Depth
   */
    void setPixelIfDepth(int x, int y, const glm::vec4& new_pixel){
        if(new_pixel.w < getPixel(x,y).w ){
            setPixel(x,y,new_pixel);
        }
    }
    /**
     * Set pixel value at coordinate if depth is greater than current pixel
     * @param x,y Coordinate. Must be in bounds.
     * @param new_pixel R,G,B,Depth
     */
    void setPixelIfDepthGreater(int x, int y, const glm::vec4& new_pixel){
        if(new_pixel.w >= getPixel(x,y).w ){
            setPixel(x,y,new_pixel);
        }
    }

    /**
     * Get width of buffer in pixels
     */
    [[nodiscard]] int getWidth() const {
        return width;
    }

    /**
     * Get height of buffer in pixels
     */
    [[nodiscard]] int getHeight() const {
        return height;
    }

    /**
     * Get pointer to raw rgba image.
     */
    [[nodiscard]] const uint8_t *getRawImage() const {
        return bit_pixels.data();
    }
};
