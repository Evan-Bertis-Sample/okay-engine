#ifndef __OKAY_H__
#define __OKAY_H__

#include <glad/glad.h>
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
    OkayTime()
        : _lastTime(HighResClock::now()), _startOfProgram(HighResClock::now()), _deltaTime(0) {
    }

    void reset() {
        _lastTime = HighResClock::now();
        _deltaTime = 0;
    }

    std::uint32_t deltaTimeMs() const {
        return _deltaTime;
    }
    float deltaTimeSec() const {
        return static_cast<float>(_deltaTime) / 1000.0f;
    }

    std::uint32_t timeSinceStartMs() const {
        return std::chrono::duration_cast<std::chrono::milliseconds>(HighResClock::now() -
                                                                     _startOfProgram)
            .count();
    }
    float timeSinceStartSec() const {
        return static_cast<float>(timeSinceStartMs()) / 1000.0f;
    }

    void updateDeltaTime() {
        TimePoint prevTime = _lastTime;
        _lastTime = HighResClock::now();
        _deltaTime =
            std::chrono::duration_cast<std::chrono::milliseconds>(_lastTime - prevTime).count();
    }

   private:
    TimePoint _lastTime;
    TimePoint _startOfProgram;
    std::uint32_t _deltaTime;
};

class OkayGame;

class OkayEngine {
   public:
    OkaySystemManager systems;
    OkayLogger logger;
    std::unique_ptr<OkayTime> time{std::make_unique<OkayTime>()};

    OkayEngine() {
    }
    ~OkayEngine() {
    }

    void shutdown() {
        _shouldRun = false;
    }
    bool shouldRun() {
        return _shouldRun;
    }

    std::size_t frameCount() const {
        return _frameCount;
    }

   private:
    bool _shouldRun{true};
    std::size_t _frameCount{0};

    friend class OkayGame;
};

extern OkayEngine Engine;

class OkayGame {
   public:
    static OkayGame create() {
        return OkayGame();
    }

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
            if (!Engine.shouldRun())
                break;
        }

        OkaySystemPool& gamePool = Engine.systems.getPool(OkaySystemScope::GAME);

        for (IOkaySystem* system : gamePool) {
            system->initialize();
            if (!Engine.shouldRun())
                break;
        }

        _onInitialize();

        Engine.time->reset();
        while (Engine.shouldRun()) {
            for (IOkaySystem* system : enginePool) {
                system->tick();
                if (!Engine.shouldRun())
                    break;
            }

            for (IOkaySystem* system : gamePool) {
                system->tick();
                if (!Engine.shouldRun())
                    break;
            }

            _onUpdate();

            for (IOkaySystem* system : enginePool) {
                system->postTick();
            }

            for (IOkaySystem* system : gamePool) {
                system->postTick();
            }
            Engine.time->updateDeltaTime();

            Engine._frameCount++;
            // Engine.logger.info("Frame {} completed.", Engine.frameCount());
        }

        for (IOkaySystem* system : enginePool) {
            system->shutdown();
        }

        for (IOkaySystem* system : gamePool) {
            system->shutdown();
        }

        for (IOkaySystem* system : Engine.systems.getPool(OkaySystemScope::LEVEL)) {
            system->shutdown();
        }

        _onShutdown();
    }

   private:
    std::function<void()> _onInitialize;
    std::function<void()> _onUpdate;
    std::function<void()> _onShutdown;

    static const std::vector<OkaySystemDescriptor> REQUIRED_SYSTEMS;
};

}  // namespace okay

#endif  // __OKAY_H__
