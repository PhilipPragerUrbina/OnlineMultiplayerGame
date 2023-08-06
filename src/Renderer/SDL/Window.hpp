//
// Created by Philip on 7/18/2023.
//

#pragma once
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <string>
#include <utility>
#include "../FrameBuffer.hpp"
#include "../../Events/EventList.hpp"

/**
 * Simple SDL window for one window at a time
 */
class Window {
private:
    int width,height;
    std::string name;

    SDL_Renderer *renderer;
    SDL_Window *window;

    SDL_Texture* frame_texture;
public:
    /**
     * Create an SDL window
     * Do not create multiple windows
     * @param width Width of window in pixels
     * @param height Height of window in pixels
     * @param name Name of window
     */
    Window(int width,int height, const std::string& name) : width(width), height(height), name(name) {
        SDL_Init(SDL_INIT_VIDEO);
        SDL_CreateWindowAndRenderer(width, height, SDL_WINDOW_RESIZABLE  , &window, &renderer);
        SDL_SetWindowTitle(window, name.c_str());
        frame_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, width, height);;
        SDL_RenderSetLogicalSize(renderer, width, height);
    }

    //todo handle resize

    /**
     * Draw a frame buffer to the screen
     * @param buffer Frame buffer to draw. Must be same size;
     */
    void drawFrameBuffer(const FrameBuffer& buffer){
        assert(buffer.getHeight() == height);
        assert(buffer.getWidth() == width);
        SDL_RenderClear(renderer);
        SDL_UpdateTexture(frame_texture , nullptr, buffer.getRawImage(), width * 4);
        SDL_RenderCopy(renderer, frame_texture , nullptr, nullptr);
        SDL_RenderPresent(renderer);
    }

    /**
     * Call this in the main loop to check if window is still open and poll events
     * @param events_list Output of all current events
     * @return True if open
     */
    bool isOpen(EventList& events_list) {
        SDL_Event event;
        std::vector<SDL_Event> events;
        while(SDL_PollEvent(&event)){
            events.push_back(event);
            if(event.type == SDL_QUIT) return false;
        };
        events_list.update(events);
        return true;
    }

    /**
     * Set the mouse to be locked
     */
    void setMouseRelative() const {
        SDL_SetRelativeMouseMode(SDL_TRUE);
    }

    /**
     * Call this in the main loop to check if window is still open and poll events
     * @return True if open
     */
    bool isOpen() {
        SDL_Event event;
        while(SDL_PollEvent(&event)){
            if(event.type == SDL_QUIT) return false;
        };
        return true;
    }

    /**
     * Close SDL
     */
    ~Window(){
        SDL_DestroyTexture(frame_texture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
    }
};
