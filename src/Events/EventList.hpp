//
// Created by Philip on 7/20/2023.
//

#pragma once

#include <vector>
#include "SDL2/SDL_events.h"

/**
 * A snapshot of what events are currently occurring
 */
struct EventList {
    /**
     * Number of keyboard keys to keep track of
     */
    static const int NUM_KEYS = 7;

    /**
     * Number of mouse buttons to keep track of
     */
    static const int NUM_MOUSE_BUTTONS = 2;

    /**
     * What specific keys to keep track of
     */
    constexpr static const SDL_KeyCode key_mappings[NUM_KEYS] = {SDLK_w,SDLK_s, SDLK_a,SDLK_d,SDLK_SPACE,SDLK_c,SDLK_e};

    /*
     * What specific mouse buttons to keep track of
     */
    constexpr static const uint8_t mouse_button_mappings[NUM_MOUSE_BUTTONS] = {SDL_BUTTON_LEFT,SDL_BUTTON_RIGHT};

    /**
     * True if key is pressed, false if not pressed
     */
    bool keys[NUM_KEYS] = {false};

    /**
     * True if a button is pressed, false if not pressed
     */
    bool mouse_buttons[NUM_MOUSE_BUTTONS] = {false};


    /**
     * Cumulative relative mouse movement
     */
    int32_t mouse_x = 0,mouse_y = 0, mouse_scroll = 0;


     /**
      * Update the event list from mapping and SDL events
      * @param current_events List of polled SDL events.
      */
    void update(const std::vector<SDL_Event>& current_events){
         for (const SDL_Event& event : current_events) {
             if(event.type == SDL_MOUSEMOTION){
                 mouse_x += event.motion.xrel;
                 mouse_y += event.motion.yrel;
             }else if(event.type == SDL_KEYUP){
                 for (int i = 0; i < NUM_KEYS; ++i) {
                     if(key_mappings[i] == event.key.keysym.sym){
                         keys[i] = false;
                         break;
                     }
                 }
             }else if(event.type == SDL_KEYDOWN){
                 for (int i = 0; i < NUM_KEYS; ++i) {
                     if(key_mappings[i] == event.key.keysym.sym){
                         keys[i] = true;
                         break;
                     }
                 }
             }else if(event.type == SDL_MOUSEBUTTONDOWN){
                 for (int i = 0; i < NUM_MOUSE_BUTTONS; ++i) {
                     if(mouse_button_mappings[i] == event.button.button){
                         mouse_buttons[i] = true;
                         break;
                     }
                 }
             }else if(event.type == SDL_MOUSEBUTTONUP){
                 for (int i = 0; i < NUM_MOUSE_BUTTONS; ++i) {
                     if(mouse_button_mappings[i] == event.button.button){
                         mouse_buttons[i] = false;
                         break;
                     }
                 }
             }else if(event.type == SDL_MOUSEWHEEL){
                 mouse_scroll += event.wheel.y;
             }
         }
    }
};
