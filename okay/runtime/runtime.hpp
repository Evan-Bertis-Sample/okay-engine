#pragma once

#include <okay/core/engine/engine.hpp>
#include <okay/util/export.hpp>

namespace okay {

class OkayRuntime {
   public:
    SystemManager systems;
    Logger logger;
};

extern OkayRuntime Runtime;

}  // namespace okay

/// @brief User-defined function
extern "C" OKAY_IMPORT void create(okay::Game* game, int argc, char* args[]);
