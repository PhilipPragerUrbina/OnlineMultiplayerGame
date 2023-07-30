//
// Created by Philip on 7/20/2023.
//

#pragma once

#include <vector>
#include "SDL2/SDL_events.h"

/**
 * A snapshot of what events are currently occuring
 */
struct EventList {
    //todo own implementation
    std::vector<SDL_Event> events;
};
