#ifndef __RENDER_PIPELINE_H__
#define __RENDER_PIPELINE_H__

#include <okay/core/renderer/render_target.hpp>
#include <okay/core/renderer/render_world.hpp>
#include <okay/core/renderer/gl.hpp>

#include <memory>
#include <vector>

namespace okay {

class Renderer;

struct RendererContext {
    Renderer& renderer;
    RenderWorld& world;
    RenderTargetPool& renderTargetPool;
};

class IRenderPass {
   public:
    virtual ~IRenderPass() = default;

    virtual const std::string_view name() const = 0;
    virtual void initialize() = 0;
    virtual void resize(int newWidth, int newHeight) = 0;
    virtual void render(const RendererContext& context) = 0;
};

class RenderPipeline {
   public:
    template <typename... Ts>
    static RenderPipeline create(std::unique_ptr<Ts>... passes) {
        RenderPipeline pipeline;
        (pipeline.addPass(std::move(passes)), ...);
        return pipeline;
    }

    // enable move semantics
    RenderPipeline() {}
    RenderPipeline(RenderPipeline&& other) : _passes(std::move(other._passes)) {}
    RenderPipeline& operator=(RenderPipeline&& other) {
        _passes = std::move(other._passes);
        return *this;
    }

    void addPass(std::unique_ptr<IRenderPass> pass) { _passes.emplace_back(std::move(pass)); }

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

    void render(const RendererContext& context) {
        for (auto& pass : _passes) {
            pass->render(context);
        }
    }

    const std::vector<std::unique_ptr<IRenderPass>>& passes() const { return _passes; }

   private:
    std::vector<std::unique_ptr<IRenderPass>> _passes;
};

};  // namespace okay

#endif  // _RENDER_PIPELINE_H__