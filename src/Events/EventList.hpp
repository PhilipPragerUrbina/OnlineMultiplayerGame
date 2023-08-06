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
    static const int NUM_BUTTONS = 7;
    constexpr static const SDL_KeyCode mappings[NUM_BUTTONS] = {SDLK_w,SDLK_s, SDLK_a,SDLK_d,SDLK_SPACE,SDLK_c,SDLK_e}; //same size as num buttons
    bool buttons[NUM_BUTTONS] = {false}; //True for down up for false
    uint32_t mouse_x = 0,mouse_y = 0; //Cumulative relative mouse movement

     /**
      * Update the event list from mapping and SDL events
      * @param current_events List of polled SDL events.
      * @param mappings Which keyboard buttons correspond to the event button slots?
      */
    void update(const std::vector<SDL_Event>& current_events){
        //todo mouse and modifier mapping
         for (const SDL_Event& event : current_events) {
            if(event.type == SDL_MOUSEMOTION){
                mouse_x += event.motion.xrel;
                mouse_y += event.motion.yrel;
            }else if(event.type == SDL_KEYUP){
                for (int i = 0; i < NUM_BUTTONS; ++i) {
                    if(mappings[i] == event.key.keysym.sym){
                        buttons[i] = false;
                        break;
                    }
                }
            }else if(event.type == SDL_KEYDOWN){
                for (int i = 0; i < NUM_BUTTONS; ++i) {
                    if(mappings[i] == event.key.keysym.sym){
                        buttons[i] = true;
                        break;
                    }
                }
            }
         }
    }
};
