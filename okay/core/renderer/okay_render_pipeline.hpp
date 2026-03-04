#ifndef __OKAY_RENDER_PIPELINE_H__
#define __OKAY_RENDER_PIPELINE_H__

#include <vector>
#include <memory>

#include <okay/core/renderer/okay_render_target.hpp>
#include <okay/core/renderer/okay_render_world.hpp>

namespace okay {

class OkayRenderer;

struct OkayRenderContext {
    OkayRenderer& renderer;
    OkayRenderWorld& world;
    OkayRenderTargetPool& renderTargetPool;
};

class IOkayRenderPass {
   public:
    virtual ~IOkayRenderPass() = default;

    virtual const std::string_view name() const = 0;
    virtual void initialize() = 0;
    virtual void resize(int newWidth, int newHeight) = 0;
    virtual void render(const OkayRenderContext& context) = 0;
};
  
class OkayRenderPipeline {
   public:
    template <typename... Ts>
    static OkayRenderPipeline create(std::unique_ptr<Ts>... passes) {
        OkayRenderPipeline pipeline;
        (pipeline.addPass(std::move(passes)), ...);
        return pipeline;
    }

    // enable move semantics
    OkayRenderPipeline() {}
    OkayRenderPipeline(OkayRenderPipeline&& other) : _passes(std::move(other._passes)) {}
    OkayRenderPipeline& operator=(OkayRenderPipeline&& other) {
        _passes = std::move(other._passes);
        return *this;
    }

    void addPass(std::unique_ptr<IOkayRenderPass> pass) {
        _passes.emplace_back(std::move(pass));
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