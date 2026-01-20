#ifndef __OKAY_H__
#define __OKAY_H__

#include <glad/gl.h>
#include <iostream>
#include <okay/core/system/okay_system.hpp>
#include <okay/core/logging/okay_logger.hpp>

#include <chrono>
#include <utility>
#include <functional>

namespace okay {

class OkayTime {
    using HighResClock = std::chrono::high_resolution_clock;
    using TimePoint = std::chrono::time_point<HighResClock>;

   public:
    OkayTime() : 
        _timeStart(HighResClock::now())
        {}

        void restart() { _timeStart = HighResClock::now(); }

        long long deltaTime() const { return _dt; }
    
        long long recalcDT() {
            TimePoint newTime = HighResClock::now();
            TimePoint oldTime = _timeStart;
            _timeStart = newTime;
            _dt =  std::chrono::duration_cast<std::chrono::milliseconds>(newTime - oldTime).count();
            return _dt;
        }

   private:
    TimePoint _timeStart;
    long long _dt;
};

class OkayEngine {
   public:
    OkaySystemManager systems;
    OkayLogger logger;
    OkayTime* time { new OkayTime() };

    OkayEngine() {}
    ~OkayEngine() {}
};

extern OkayEngine Engine;

class OkayGame {
   public:
    static OkayGame create() { return OkayGame(); }

    // ...variadic function that takes in std::unique_ptr<OkaySystem<T>> for each system type
    template <typename... Systems>
    OkayGame& addSystems(std::unique_ptr<Systems>... systems) {
        (Engine.systems.registerSystem(std::move(systems)), ...);
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
        bool allRequiredSystems = true;
        for (OkaySystemDescriptor systemDescriptor : REQUIRED_SYSTEMS) {
            if (!Engine.systems.hasSystem(systemDescriptor.SysId)) {
                Engine.logger.error("Missing required system {}", systemDescriptor.SystemName);
                allRequiredSystems = false;
            }
        }

        if (!allRequiredSystems) {
            return;
        }

        OkaySystemPool& enginePool = Engine.systems.getPool(OkaySystemScope::ENGINE);

        for (IOkaySystem* system : enginePool) {
            system->initialize();
        }

        OkaySystemPool& gamePool = Engine.systems.getPool(OkaySystemScope::GAME);

        for (IOkaySystem* system : gamePool) {
            system->initialize();
        }

        _onInitialize();

        Engine.time->restart();

        while (_shouldRun) {
            Engine.time->recalcDT();

            for (IOkaySystem* system : enginePool) {
                system->tick();
            }

            for (IOkaySystem* system : gamePool) {
                system->tick();
            }

            _onUpdate();
        }
    }

   private:
    std::function<void()> _onInitialize;
    std::function<void()> _onUpdate;
    std::function<void()> _onShutdown;

    static const std::vector<OkaySystemDescriptor> REQUIRED_SYSTEMS;

    bool _shouldRun = true;
};

}  // namespace okay

#endif  // __OKAY_H__
