#ifndef __OKAY_H__
#define __OKAY_H__

#include <functional>
#include <iostream>
#include <okay/core/system/okay_system.hpp>
#include <string>

namespace okay {

class OkayEngine {
   public:
    OkaySystemManager systems;

    void initialize() {
        
    }
};

extern OkayEngine Engine;

template <typename GameConfigT>
class OkayGame {
   public:
    static OkayGame create() {
        return OkayGame();
    }
    
    // ...variadic function that takes in std::unique_ptr<OkaySystem<T>> for each system type
    template <typename... Systems>
    OkayGame& addSystems(std::unique_ptr<Systems>... systems) {
        (Engine.systems.template registerSystem(std::move(systems)), ...);
        return *this;
    }

    OkayGame& onInitialize(std::function<void(GameConfigT&)> callback) {
        _onInitialize = std::move(callback);
        return *this;
    }

    OkayGame& onUpdate(std::function<void(GameConfigT&)> callback) {
        _onUpdate = std::move(callback);
        return *this;
    }

    OkayGame& onShutdown(std::function<void(GameConfigT&)> callback) {
        _onShutdown = std::move(callback);
        return *this;
    }

    void run(GameConfigT& config) {
        OkaySystemPool &enginePool = Engine.systems.getPool(OkaySystemScope::ENGINE);

        for (IOkaySystem* system : enginePool) {
            system->initialize();
        }

        _onInitialize(config);
    }

   private:
    std::function<void(GameConfigT&)> _onInitialize;
    std::function<void(GameConfigT&)> _onUpdate;
    std::function<void(GameConfigT&)> _onShutdown;
};

}  // namespace okay

#endif  // __OKAY_H__