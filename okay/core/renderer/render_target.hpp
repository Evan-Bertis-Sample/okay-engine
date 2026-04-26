#ifndef __RENDER_TARGET_H__
#define __RENDER_TARGET_H__

#include <okay/core/renderer/gl.hpp>
#include <okay/core/util/option.hpp>

#include <string>
#include <vector>

namespace okay {

struct RenderTargetConfig {
    const std::string& name;
    int width = 800;
    int height = 600;
};

class RenderTarget {
   public:
    RenderTarget() : RenderTarget({""}) {}

    RenderTarget(const RenderTargetConfig& config) : _config(config) {}

    const std::string& name() const {
        return _config.name;
    }

   private:
    RenderTargetConfig _config;
};

class RenderTargetPool {
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

    RenderTargetPool(int surfaceWidth, int surfaceHeight)
        : _surfaceWidth(surfaceWidth), _surfaceHeight(surfaceHeight) {}

    TargetHandle addRenderTarget(const RenderTargetConfig& config) {
        TargetHandle handle{_targets.size()};
        _targets.emplace_back(config);
        return handle;
    }

    const Option<RenderTarget> getRenderTarget(const TargetHandle& handle) const {
        if (handle.index >= _targets.size()) {
            return Option<RenderTarget>::none();
        }
        return Option<RenderTarget>::some(_targets[handle.index]);
    }

    void initializeBuiltins() {}

   private:
    std::vector<RenderTarget> _targets;
    int _surfaceWidth;
    int _surfaceHeight;
};

};  // namespace okay

#endif  // _RENDER_TARGET_H__