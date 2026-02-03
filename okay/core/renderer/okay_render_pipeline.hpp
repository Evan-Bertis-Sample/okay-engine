#ifndef __OKAY_RENDER_PIPELINE_H__
#define __OKAY_RENDER_PIPELINE_H__

#include <vector>
#include <memory>

#include <okay/core/renderer/okay_render_target.hpp>
#include <okay/core/renderer/okay_render_world.hpp>

namespace okay {

struct OkayRenderContext {
    OkayRenderWorld& world;
    OkayRenderTargetPool& renderTargetPool;
};

class IOkayRenderPass {
   public:
    virtual ~IOkayRenderPass() = default;

    virtual const std::string& name() const = 0;
    virtual void initialize() = 0;
    virtual void resize(int newWidth, int newHeight) = 0;
    virtual void render(const OkayRenderContext& context) = 0;
};

class OkayRenderPipeline {
   public:
    void addPass(std::unique_ptr<IOkayRenderPass> pass) {
        _passes.push_back(std::move(pass));
    }

    void initialize() {
        for (auto& pass : _passes) {
            pass->initialize();
        }
    }

    void resize(int newWidth, int newHeight) {
        for (auto& pass : _passes) {
            pass->resize(newWidth, newHeight);
        }
    }

    void render(const OkayRenderContext& context) {
        for (auto& pass : _passes) {
            pass->render(context);
        }
    }

    const std::vector<std::unique_ptr<IOkayRenderPass>>& passes() const { return _passes; }

   private:
    std::vector<std::unique_ptr<IOkayRenderPass>> _passes;
};

};  // namespace okay

#endif  // __OKAY_RENDER_PIPELINE_H__