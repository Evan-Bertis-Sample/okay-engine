#ifndef __RUNTIME_HPP__
#define __RUNTIME_HPP__

#include <okay/core/engine/engine.hpp>
#include <okay/core/util/export.hpp>

namespace okay {

class OkayRuntime {
   public:
    SystemManager systems;
    Logger logger;
};

extern OkayRuntime Runtime;

}  // namespace okay

/// @brief User-defined function
extern "C" OKAY_IMPORT okay::Game create(int argc, char* args[]);

#endif  // __RUNTIME_HPP__
