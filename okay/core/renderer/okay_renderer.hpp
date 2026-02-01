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

namespace okay {

struct OkayRendererSettings {
    SurfaceConfig SurfaceConfig;
};

class OkayRenderer : public OkaySystem<OkaySystemScope::ENGINE> {
   public:
    static std::unique_ptr<OkayRenderer> create(const OkayRendererSettings& settings) {
        return std::make_unique<OkayRenderer>(settings);
    }

    OkayRenderer(const OkayRendererSettings& settings)
        : _settings(settings), _surface(std::make_unique<Surface>(settings.SurfaceConfig)) {}

    using TestMaterialUniforms =
        OkayMaterialUniformCollection<OkayMaterialUniform<glm::vec3, FixedString("u_color")>>;

    OkayShader<TestMaterialUniforms> shader;

    void initialize() override;

    void postInitialize() override;

    void tick() override;

    void postTick() override;

    void shutdown() override;

    void setBoxPosition(const glm::vec3& position);

   private:
    OkayRendererSettings _settings;
    std::unique_ptr<Surface> _surface;

    OkayMeshBuffer _modelBuffer;
    OkayMesh _model;
};

}  // namespace okay

#endif  // __OKAY_RENDERER_H__
