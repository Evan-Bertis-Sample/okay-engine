#ifndef __OKAY_LEVEL_H__
#define __OKAY_LEVEL_H__

#include <functional>
#include <okay/core/system/okay_system.hpp>
#include <memory>
#include <functional>

namespace okay {

class OkayLevel {
   public:
    static OkayLevel load() { return OkayLevel(); }

    template <typename... Systems>
    OkayLevel& addLevelSystems(std::unique_ptr<Systems>... systems) {
        (_levelSystems.template registerSystem<Systems>(std::move(systems)), ...);
        return *this;
    };

    OkayLevel& onInitalize(std::function<void()> callback) {
        _onInitialize = std::move(callback);
        return *this;
    };

    OkayLevel& onUpdate(std::function<void()> callback) {
        _onUpdate = std::move(callback);
        return *this;
    }

    OkayLevel& onShutdown(std::function<void()> callback) {
        _onShutdown = std::move(callback);
        return *this;
    }

    OkaySystemPool& getLevelSystemsPool() { return _levelSystems; }

   private:
    OkaySystemPool _levelSystems;

    std::function<void()> _onInitialize;
    std::function<void()> _onUpdate;
    std::function<void()> _onShutdown;
};

};  // namespace okay

#endif  // __OKAY_LEVEL_H__