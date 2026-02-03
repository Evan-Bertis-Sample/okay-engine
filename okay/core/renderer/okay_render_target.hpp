#ifndef __OKAY_RENDER_TARGET_H__
#define __OKAY_RENDER_TARGET_H__

#include <string>
#include <vector>

#include <okay/core/util/option.hpp>

namespace okay {

struct OkayRenderTargetConfig {
    const char* name;
    int width = 800;
    int height = 600;
};

class OkayRenderTarget {
   public:
    OkayRenderTarget(const OkayRenderTargetConfig& config)
        : _name(config.name), _width(config.width), _height(config.height) {
    }

    const std::string& name() const {
        return _name;
    }

   private:
    const std::string _name;
    int _width;
    int _height;
};

class OkayRenderTargetPool {
   public:
    struct TargetHandle {
        std::size_t index;
    };

    enum class BuiltinTargetID : std::size_t { COLOR = 0, DEPTH = 1, __COUNT };

    static constexpr TargetHandle colorTarget() {
        return TargetHandle{static_cast<int>(BuiltinTargetID::COLOR)};
    }

    static constexpr TargetHandle depthTarget() {
        return TargetHandle{static_cast<int>(BuiltinTargetID::DEPTH)};
    }

    OkayRenderTargetPool(int surfaceWidth, int surfaceHeight)
        : _surfaceWidth(surfaceWidth), _surfaceHeight(surfaceHeight) {
    }

    TargetHandle addRenderTarget(const OkayRenderTargetConfig& config) {
        TargetHandle handle{_targets.size()};
        _targets.emplace_back(config);
        return handle;
    }

    const Option<OkayRenderTarget> getRenderTarget(const TargetHandle& handle) const {
        if (handle.index >= _targets.size()) {
            return Option<OkayRenderTarget>::none();
        }
        return Option<OkayRenderTarget>::some(_targets[handle.index]);
    }

   private:
    void initializeBuiltins();

    std::vector<OkayRenderTarget> _targets;
    int _surfaceWidth;
    int _surfaceHeight;
};

};  // namespace okay

#endif  // __OKAY_RENDER_TARGET_H__