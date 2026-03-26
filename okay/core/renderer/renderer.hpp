#ifndef __OKAY_RENDERER_H__
#define __OKAY_RENDERER_H__

#include <cstdint>
#include <okay/core/engine/engine.hpp>
#include <okay/core/asset/generic/shader_loader.hpp>
#include <okay/core/asset/asset.hpp>
#include <okay/core/renderer/material.hpp>
#include <okay/core/renderer/mesh.hpp>
#include <okay/core/renderer/primitive.hpp>
#include <okay/core/renderer/surface.hpp>
#include <okay/core/engine/system.hpp>
#include <okay/core/util/singleton.hpp>
#include <okay/core/renderer/render_world.hpp>
#include <GLFW/glfw3.h>
#include "render_pipeline.hpp"
#include "render_target.hpp"

namespace okay {

struct OkayRendererSettings {
    SurfaceConfig surfaceConfig;
    OkayRenderPipeline pipeline;
};

class OkayRenderer : public OkaySystem<OkaySystemScope::ENGINE> {
   public:
    static std::unique_ptr<OkayRenderer> create(OkayRendererSettings settings) {
        return std::make_unique<OkayRenderer>(std::move(settings));
    }

    explicit OkayRenderer(OkayRendererSettings settings)
        : _surfaceConfig(settings.surfaceConfig),
          _surface(std::make_unique<Surface>(settings.surfaceConfig)),
          _renderTargetPool(settings.surfaceConfig.width, settings.surfaceConfig.height),
          _pipeline(std::move(settings.pipeline)) {}

    OkayShader shader;

    void initialize() override;
    void postInitialize() override;
    void tick() override;
    void postTick() override;
    void shutdown() override;

    OkayRenderWorld& world() { return _world; }
    OkayMeshBuffer& meshBuffer() { return _meshBuffer; }
    OkayRenderTargetPool& renderTargetPool() { return _renderTargetPool; }
    OkayMaterialRegistry& materialRegistry() { return _materialRegistry; }

    uint32_t width() const { return _surfaceConfig.width; }
    uint32_t height() const { return _surfaceConfig.height; }

    GLFWwindow* getSurfaceWindow() { return reinterpret_cast<GLFWwindow*>(_surface->getWindow()); }

   private:
    SurfaceConfig _surfaceConfig;
    OkayRenderWorld _world;
    OkayMeshBuffer _meshBuffer;
    OkayRenderPipeline _pipeline;
    OkayRenderTargetPool _renderTargetPool;
    OkayMaterialRegistry _materialRegistry;
    std::unique_ptr<Surface> _surface;
};

}  // namespace okay

#endif  // __OKAY_RENDERER_H__
