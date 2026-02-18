#ifndef __OKAY_RENDERER_H__
#define __OKAY_RENDERER_H__

#include <okay/core/okay.hpp>
#include <okay/core/asset/generic/shader_loader.hpp>
#include <okay/core/asset/okay_asset.hpp>
#include <okay/core/renderer/okay_material.hpp>
#include <okay/core/renderer/okay_mesh.hpp>
#include <okay/core/renderer/okay_primitive.hpp>
#include <okay/core/renderer/okay_surface.hpp>
#include <okay/core/system/okay_system.hpp>
#include <okay/core/util/singleton.hpp>
#include <okay/core/renderer/okay_render_world.hpp>
#include "okay_render_pipeline.hpp"
#include "okay_render_target.hpp"

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
          _pipeline(std::move(settings.pipeline)) {
    }

    OkayShader shader;

    void initialize() override;
    void postInitialize() override;
    void tick() override;
    void postTick() override;
    void shutdown() override;

    OkayRenderWorld& world() {
        return _world;
    }
    OkayMeshBuffer& meshBuffer() {
        return _meshBuffer;
    }
    OkayRenderTargetPool& renderTargetPool() {
        return _renderTargetPool;
    }
    OkayMaterialRegistry& materialRegistry() {
        return _materialRegistry;
    }

   private:
    SurfaceConfig _surfaceConfig;
    OkayRenderWorld _world;
    OkayMeshBuffer _meshBuffer;
    OkayRenderPipeline _pipeline;
    OkayRenderTargetPool _renderTargetPool;
    OkayMaterialRegistry _materialRegistry;
    std::unique_ptr<Surface> _surface;

    // dirty flags
    bool _meshBufferDirty{false};
};

}  // namespace okay

#endif  // __OKAY_RENDERER_H__
