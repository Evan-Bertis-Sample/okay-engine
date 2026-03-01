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
#include <glm/ext/quaternion_trigonometric.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/gtc/random.hpp>
#include "glm/ext/matrix_transform.hpp"
#include "okay/core/renderer/okay_mesh.hpp"
#include "okay/core/util/result.hpp"

static void __gameInitialize();
static void __gameUpdate();
static void __gameShutdown();

static okay::OkayRenderEntity g_sun;
static okay::OkayRenderEntity g_planet;
static okay::OkayRenderEntity g_moon;

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
            okay::primitives::uvSphere()
            .radiusSet(0.5f)
            .segmentsSet(20)
            .ringsSet(20)
            .build());

    okay::OkayMesh cube = 
        renderer->meshBuffer().addMesh(
            okay::primitives::box()
            .sizeSet({0.3f, 0.3f, 0.3f})
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
    g_sun = renderer->world().addRenderEntity(
        okay::OkayTransform(),
        mat, icoSphere
    );

    g_planet = renderer->world().addRenderEntity(
        okay::OkayTransform({ -2.0f, 0.0f, 0.0f }, { 0.3f, 0.3f, 0.3f }),
        mat, icoSphere
    );

    g_moon = renderer->world().addRenderEntity(
        okay::OkayTransform({ 1.0f, 0.0f, 0.0f } , { 0.2f, 0.2f, 0.2f }),
        mat, icoSphere
    );

    renderer->world().addChild(g_sun, g_planet);
    renderer->world().addChild(g_planet, g_moon);

    // add a bunch of cubes in a circle around the origin
    const int numCubes = 1000;
    for (int i = 0; i < numCubes; ++i) {
        // random unit vector
        glm::vec3 pos = glm::gaussRand(glm::vec3(0.0f), glm::vec3(1.0f));
        pos = glm::normalize(pos);

        pos *= glm::linearRand(10.0f, 20.0f);

        glm::vec3 scale = glm::gaussRand(glm::vec3(0.0f), glm::vec3(0.5f));
        // add to world
        renderer->world().addRenderEntity(
            okay::OkayTransform(pos, scale, glm::quat()),
            mat, cube
        );  
    }
}

static void __gameUpdate() {
    // Move the whole solar system
    g_sun->transform.position.y = sin(okay::Engine.time->timeSinceStartSec()) * 0.5f;
    
    // Rotate the sun
    g_sun->transform.rotation = glm::angleAxis(
        glm::radians(45.0f) * okay::Engine.time->timeSinceStartSec() * 1.0f, glm::vec3(0.0f, 1.0f, 0.0f));

    // Rotate the planet
    g_planet->transform.rotation = glm::angleAxis(
        glm::radians(45.0f) * okay::Engine.time->timeSinceStartSec() * 1.5f, glm::vec3(0.0f, 1.0f, 0.0f));

    // Rotate the moon
    g_moon->transform.rotation = glm::angleAxis(
        glm::radians(45.0f) * okay::Engine.time->timeSinceStartSec() * 2.0f, glm::vec3(0.0f, 1.0f, 0.0f));

    // move the camera in a circle, always looking at the origin
    okay::OkayRenderer* renderer = okay::Engine.systems.getSystemChecked<okay::OkayRenderer>();
    float theta = okay::Engine.time->timeSinceStartSec() * 0.05f * glm::pi<float>();
    glm::vec3 pos = glm::vec3(
        sin(theta) * 5.0f,
        0.0f,
        cos(theta) * 5.0f
    );
    // rotation much look at origin
    renderer->world().camera().transform.position = pos;
    renderer->world().camera().lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
}

static void __gameShutdown() {
    // Cleanup logic before game shutdown
    okay::Engine.logger.info("Game shutdown.");
}
