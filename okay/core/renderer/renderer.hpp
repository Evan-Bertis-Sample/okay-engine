#ifndef __RENDERER_H__
#define __RENDERER_H__

#include "imgui_impl.hpp"
#include "render_pipeline.hpp"
#include "render_target.hpp"

#include <okay/core/asset/asset.hpp>
#include <okay/core/asset/generic/shader_loader.hpp>
#include <okay/core/engine/engine.hpp>
#include <okay/core/engine/system.hpp>
#include <okay/core/renderer/material.hpp>
#include <okay/core/renderer/mesh.hpp>
#include <okay/core/renderer/primitive.hpp>
#include <okay/core/renderer/render_world.hpp>
#include <okay/core/renderer/surface.hpp>
#include <okay/core/util/singleton.hpp>

#include <cstdint>

namespace okay {

struct RendererSettings {
    SurfaceConfig surfaceConfig;
    RenderPipeline pipeline;
    bool enableIMGUI{true};
};

class Renderer : public System<SystemScope::ENGINE> {
   public:
    static std::unique_ptr<Renderer> create(RendererSettings settings) {
        return std::make_unique<Renderer>(std::move(settings));
    }

    explicit Renderer(RendererSettings settings)
        : _surfaceConfig(settings.surfaceConfig),
          _surface(std::make_unique<Surface>(settings.surfaceConfig)),
          _renderTargetPool(settings.surfaceConfig.width, settings.surfaceConfig.height),
          _pipeline(std::move(settings.pipeline)),
          _imguiEnabled(settings.enableIMGUI) {}

    Shader shader;

    void initialize() override;
    void postInitialize() override;
    void preTick() override;
    void tick() override;
    void postTick() override;
    void shutdown() override;

    RenderWorld& world() { return _world; }
    MeshBuffer& meshBuffer() { return _meshBuffer; }
    RenderTargetPool& renderTargetPool() { return _renderTargetPool; }
    MaterialRegistry& materialRegistry() { return _materialRegistry; }

    uint32_t width() const { return _surfaceConfig.width; }
    uint32_t height() const { return _surfaceConfig.height; }

    void* getSurfaceWindow() { return _surface->getWindow(); }

   private:
    SurfaceConfig _surfaceConfig;
    RenderWorld _world;
    MeshBuffer _meshBuffer;
    RenderPipeline _pipeline;
    RenderTargetPool _renderTargetPool;
    MaterialRegistry _materialRegistry;
    std::unique_ptr<Surface> _surface;
    std::unique_ptr<IMGUIImpl> _imguiImpl;
    bool _imguiEnabled{false};
    bool _imguiInitialized{false};
};

}  // namespace okay

#endif  // _RENDERER_H__
