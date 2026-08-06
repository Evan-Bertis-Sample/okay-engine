#ifndef __ENGINE_H__
#define __ENGINE_H__

#include <okay/core/engine/logger.hpp>
#include <okay/core/engine/reload.hpp>
#include <okay/core/engine/resource.hpp>
#include <okay/core/engine/system.hpp>
#include <okay/core/engine/time.hpp>

#include <functional>
#include <source_location>
#include <utility>

namespace okay {
class Game;

class OkayEngine {
   public:
    SystemManager systems;
    ResourceManager resources;
    Logger logger;
    std::unique_ptr<Time> time{std::make_unique<Time>()};

    OkayEngine() {}
    ~OkayEngine() {}

    void shutdown(std::source_location loc = std::source_location::current()) {
        _shouldRun = false;
        _shutdownLoc = loc;
    }

    bool shouldRun() {
        return _shouldRun;
    }

    std::size_t frameCount() const {
        return _frameCount;
    }

    std::source_location shutdownLoc() const {
        return _shutdownLoc;
    }

   private:
    bool _shouldRun{true};
    std::size_t _frameCount{0};
    std::source_location _shutdownLoc;

    friend class Game;
};

extern OkayEngine Engine;

class Game {
   public:
    static Game create() {
        return Game();
    }

    template <typename... Systems>
    Game& addSystems(std::unique_ptr<Systems>... systems) {
        (Engine.systems.registerSystem(std::move(systems)), ...);
        return *this;
    }

    template <typename... Resources>
    Game& addResources(std::unique_ptr<Resources>... resources) {
        (Engine.resources.addResource(std::move(resources)), ...);
        return *this;
    }

    Game& onInitialize(std::function<void()> callback);
    Game& onUpdate(std::function<void()> callback);
    Game& onShutdown(std::function<void()> callback);

    bool initialize();
    void prepareForReload(ReloadContext& context);
    void reload(ReloadContext& context);
    void tick();
    void shutdown();
    void run();

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

    T* operator->() const {
        return get();
    }
};

template <typename T>
    requires ScopedResource<T>
struct ResourceParameter {
    T* system{nullptr};
    ResourceParameter(T* system) : system(system) {}

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

    T* operator->() const {
        return get();
    }
};

template <typename Derived, ResourceScope Scope>
struct ScopedSingleton : Resource<Scope> {
    static Derived& instance() {
        if (Engine.resources.hasResource<Derived>()) {
            return *Engine.resources.getResource<Derived>().value();
        }

        Engine.logger.debug(
            "Creating instance of scoped singleton: {}", ResourceDescriptor::getSysName<Derived>());
        Engine.resources.addResource<Derived>();
        return *Engine.resources.getResource<Derived>().value();
    };
};

};  // namespace okay

#endif  // __ENGINE_H__
