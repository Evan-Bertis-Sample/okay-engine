#include <okay/core/okay.hpp>
#include <okay/core/renderer/okay_renderer.hpp>
#include <okay/core/renderer/okay_surface.hpp>
#include <okay/core/level/okay_level_manager.hpp>
#include <okay/core/asset/okay_asset.hpp>
#include <okay/core/logging/okay_logger.hpp>
#include <okay/core/okay_renderer.hpp>

#include <utility>
#include <okay/core/renderer/okay_material.hpp>
#include <okay/core/renderer/okay_render_pipeline.hpp>
#include <okay/core/renderer/okay_uniform.hpp>
#include <okay/core/renderer/passes/scene_pass.hpp>
#include "glm/ext/quaternion_trigonometric.hpp"
#include "glm/ext/vector_float3.hpp"
#include "okay/core/util/result.hpp"

static void __gameInitialize();
static void __gameUpdate();
static void __gameShutdown();

static okay::OkayRenderEntity g_renderEntity;

int main() {
    okay::SurfaceConfig surfaceConfig;
    okay::Surface surface(surfaceConfig);

    okay::OkayRendererSettings rendererSettings{
        .surfaceConfig = surfaceConfig,
        .pipeline = okay::OkayRenderPipeline::create(
            std::make_unique<okay::ScenePass>()
        )
    };
     
    auto renderer = okay::OkayRenderer::create(std::move(rendererSettings));

    okay::OkayLevelManagerSettings levelManagerSettings;
    auto levelManager = okay::OkayLevelManager::create(levelManagerSettings);

    okay::OkayGame::create()
        .addSystems(std::move(renderer),
                    std::move(levelManager),
                    std::make_unique<okay::OkayAssetManager>())
        .onInitialize(__gameInitialize)
        .onUpdate(__gameUpdate)
        .onShutdown(__gameShutdown)
        .run();

    return 0;
}

static void __gameInitialize() {
    // Additional game initialization logic
    okay::OkayRenderer* renderer = okay::Engine.systems.getSystemChecked<okay::OkayRenderer>();

    okay::OkayMesh mesh =
        renderer->meshBuffer().addMesh(
            okay::primitives::icoSphere()
            .radiusSet(0.1f)
            .withCenter({0.0f, 0.0f, -2.0f})
            .subdivisionsSet(4)
            .build());

    okay::Failable res = renderer->meshBuffer().bindMeshData();
    if (res.isError()) {
        okay::Engine.logger.error("Failed to bind mesh data: {}", res.error());
        return;
    }

    okay::OkayAssetManager* assetManager =
        okay::Engine.systems.getSystemChecked<okay::OkayAssetManager>();
    auto shaderLoadRes = assetManager->loadEngineAssetSync<okay::OkayShader>("shaders/test");

    if (shaderLoadRes.isError()) {
        okay::Engine.logger.error("Failed to load shader: {}", shaderLoadRes.error());
        return;
    }

    okay::OkayShader shader = shaderLoadRes.value().asset;

    okay::Engine.logger.info("Vertex shader: {}", shader.vertexShader);
    okay::Engine.logger.info("Fragment shader: {}", shader.fragmentShader);

    okay::OkayMaterialHandle mat = renderer->materialRegistry().registerMaterial(shader, std::make_unique<okay::BaseMaterialUniforms>());
    
    glm::vec3 startingPos = { 0.0f, 0.0f, 1.0f };
    glm::vec3 startingRot = { 0.0f, 0.0f, 0.0f };
    glm::vec3 startingScale = { 1.0f, 1.0f, 1.0f };

    g_renderEntity = renderer->world().addRenderEntity(
        okay::OkayTransform(startingPos, startingScale, glm::quat(startingRot)),
        mat, mesh
    );
}

static void __gameUpdate() {
    // Game update logic
    g_renderEntity->transform.position.x += 0.1f;
}

static void __gameShutdown() {
    // Cleanup logic before game shutdown
    okay::Engine.logger.info("Game shutdown.");
}
