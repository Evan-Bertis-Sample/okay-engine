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
#include "okay/core/renderer/okay_mesh.hpp"
#include "okay/core/util/result.hpp"

static void __gameInitialize();
static void __gameUpdate();
static void __gameShutdown();

static okay::OkayRenderEntity g_renderEntityA;
static okay::OkayRenderEntity g_renderEntityB;

int main() {
    okay::SurfaceConfig surfaceConfig;
    surfaceConfig.width = 800;
    surfaceConfig.height = 480;
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

    okay::OkayMesh icoSphere =
        renderer->meshBuffer().addMesh(
            okay::primitives::icoSphere()
            .radiusSet(0.1f)
            .subdivisionsSet(4)
            .build());

    okay::OkayMesh cube = 
        renderer->meshBuffer().addMesh(
            okay::primitives::box()
            .sizeSet({0.1f, 0.1f, 0.1f})
            .build()
        );

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
    okay::OkayMaterialHandle mat = renderer->materialRegistry().registerMaterial(shader, std::make_unique<okay::BaseMaterialUniforms>());
    
    glm::vec3 startingPos = { 0.0f, 0.0f, -2.0f };
    glm::vec3 startingRot = { 0.0f, 0.0f, 0.0f };
    glm::vec3 startingScale = { 1.0f, 1.0f, 1.0f };

    g_renderEntityA = renderer->world().addRenderEntity(
        okay::OkayTransform(startingPos, startingScale, glm::quat(startingRot)),
        mat, icoSphere
    );

    g_renderEntityB = renderer->world().addRenderEntity(
        okay::OkayTransform({ -1.0f, 0.0f, 0.0f }, startingScale, glm::quat(startingRot)),
        mat, cube
    );

    renderer->world().addChild(g_renderEntityA, g_renderEntityB);

    // add a bunch of cubes in a circle around the origin
    const int numCubes = 1000;
    for (int i = 0; i < numCubes; ++i) {
        // random unit vector
        glm::vec3 pos = glm::normalize(glm::vec3(
            (float)rand() / (float)RAND_MAX - 0.5f,
            (float)rand() / (float)RAND_MAX - 0.5f,
            (float)rand() / (float)RAND_MAX - 0.5f
        ));

        // displace by random
        pos *= ((float)rand() / (float)RAND_MAX) * 100.0f;

        // add to world
        renderer->world().addRenderEntity(
            okay::OkayTransform(pos, startingScale, glm::quat(startingRot)),
            mat, cube
        );  
    }
}

static void __gameUpdate() {
    // Game update logic
    g_renderEntityA->transform.position.x += 0.1f * okay::Engine.time->deltaTimeSec();
    g_renderEntityA->transform.rotation = glm::angleAxis(
        glm::radians(45.0f) * okay::Engine.time->timeSinceStartSec(), glm::vec3(0.0f, 1.0f, 0.0f));
}

static void __gameShutdown() {
    // Cleanup logic before game shutdown
    okay::Engine.logger.info("Game shutdown.");
}
