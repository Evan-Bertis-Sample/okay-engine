#ifndef __ENGINE_H__
#define __ENGINE_H__

#include <okay/core/engine/logger.hpp>
#include <okay/core/engine/system.hpp>
#include <okay/core/engine/time.hpp>

#include <functional>
#include <utility>

namespace okay {
class Game;

class OkayEngine {
   public:
    SystemManager systems;
    Logger logger;
    std::unique_ptr<Time> time{std::make_unique<Time>()};

    OkayEngine() {}
    ~OkayEngine() {}

    void shutdown() { _shouldRun = false; }
    bool shouldRun() { return _shouldRun; }

    std::size_t frameCount() const { return _frameCount; }

   private:
    bool _shouldRun{true};
    std::size_t _frameCount{0};

    friend class Game;
};

extern OkayEngine Engine;

class Game {
   public:
    static Game create() { return Game(); }

    // ...variadic function that takes in std::unique_ptr<OkaySystem<T>> for each system type
    template <typename... Systems>
    Game& addSystems(std::unique_ptr<Systems>... systems) {
        (Engine.systems.registerSystem(std::move(systems)), ...);
        return *this;
    }

    Game& onInitialize(std::function<void()> callback) {
        _onInitialize = std::move(callback);
        return *this;
    }

    Game& onUpdate(std::function<void()> callback) {
        _onUpdate = std::move(callback);
        return *this;
    }

    Game& onShutdown(std::function<void()> callback) {
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

        SystemPool& enginePool = Engine.systems.getPool(SystemScope::ENGINE);

        for (ISystem* system : enginePool) {
            system->initialize();
            if (!Engine.shouldRun())
                break;
        }

        SystemPool& gamePool = Engine.systems.getPool(SystemScope::GAME);

        for (ISystem* system : gamePool) {
            system->initialize();
            if (!Engine.shouldRun())
                break;
        }

        // TODO: Make a level manager that handles transitioning between levels
        // right now we are assuming one level, which is incorrect

        SystemPool& levelPool = Engine.systems.getPool(SystemScope::LEVEL);
        for (ISystem* system : levelPool) {
            system->initialize();
            if (!Engine.shouldRun())
                break;
        }

        _onInitialize();

        for (ISystem* system : enginePool) {
            system->postInitialize();
        }

        for (ISystem* system : gamePool) {
            system->postInitialize();
        }

        for (ISystem* system : levelPool) {
            system->postInitialize();
        }

        Engine.time->reset();
        while (Engine.shouldRun()) {
            for (ISystem* system : enginePool) {
                system->preTick();
            }

            for (ISystem* system : gamePool) {
                system->preTick();
            }

            for (ISystem* system : levelPool) {
                system->preTick();
            }

            for (ISystem* system : enginePool) {
                system->tick();
                if (!Engine.shouldRun())
                    break;
            }

            for (ISystem* system : gamePool) {
                system->tick();
                if (!Engine.shouldRun())
                    break;
            }

            for (ISystem* system : levelPool) {
                system->tick();
                if (!Engine.shouldRun())
                    break;
            }

            _onUpdate();

            for (ISystem* system : enginePool) {
                system->postTick();
            }

            for (ISystem* system : gamePool) {
                system->postTick();
            }

            for (ISystem* system : levelPool) {
                system->postTick();
            }

            Engine.time->updateDeltaTime();

            Engine._frameCount++;
            // Engine.logger.info("Frame {} completed.", Engine.frameCount());
        }

        for (ISystem* system : enginePool) {
            system->shutdown();
        }

        for (ISystem* system : gamePool) {
            system->shutdown();
        }

        for (ISystem* system : Engine.systems.getPool(SystemScope::LEVEL)) {
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

template <typename T>
struct SystemParameter {
    T* system{nullptr};
    SystemParameter(T* system) : system(system) {}

    T* get() const {
        if (system == nullptr) {
            return Engine.systems.getSystemChecked<T>();
        }

        return system;
    }

    T& operator*() const {
        T& system = *get();
        return system;
    }

    T* operator->() const { return get(); }
};

};  // namespace okay

#endif  // __ENGINE_H__