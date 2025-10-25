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

    void initialize() {}
};

extern OkayEngine Engine;

class OkayGame {
   public:
    static OkayGame create() { return OkayGame(); }

    // ...variadic function that takes in std::unique_ptr<OkaySystem<T>> for each system type
    template <typename... Systems>
    OkayGame& addSystems(std::unique_ptr<Systems>... systems) {
        (Engine.systems.template registerSystem(std::move(systems)), ...);
        return *this;
    }

    OkayGame& onInitialize(std::function<void()> callback) {
        _onInitialize = std::move(callback);
        return *this;
    }

    OkayGame& onUpdate(std::function<void()> callback) {
        _onUpdate = std::move(callback);
        return *this;
    }

    OkayGame& onShutdown(std::function<void()> callback) {
        _onShutdown = std::move(callback);
        return *this;
    }

    void run() {
        // check for required systems
        for (std::size_t systemHash : _REQUIRED_SYSTEMS) {
            if (!Engine.systems.hasSystem(systemHash)) {
                std::cout << "missing required system...\n";
                return;
            }
        }

        OkaySystemPool& enginePool = Engine.systems.getPool(OkaySystemScope::ENGINE);

        for (IOkaySystem* system : enginePool) {
            system->initialize();
        }

        OkaySystemPool& gamePool = Engine.systems.getPool(OkaySystemScope::GAME);

        for (IOkaySystem* system : enginePool) {
            system->initialize();
        }

        _onInitialize();

        while (_shouldRun) {
            for (IOkaySystem* system : enginePool) {
                system->tick();
            }

            for (IOkaySystem* system : enginePool) {
                system->tick();
            }

            _onUpdate();
        }
    }

   private:
    std::function<void()> _onInitialize;
    std::function<void()> _onUpdate;
    std::function<void()> _onShutdown;

    static const std::vector<std::size_t> _REQUIRED_SYSTEMS;

    bool _shouldRun = true;
};

}  // namespace okay

#endif  // __OKAY_H__