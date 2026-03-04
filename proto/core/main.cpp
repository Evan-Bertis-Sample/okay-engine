#include <okay/core/okay.hpp>
#include <okay/core/renderer/okay_renderer.hpp>
#include <okay/core/renderer/okay_surface.hpp>
#include <okay/core/renderer/okay_text.hpp>
#include <okay/core/level/okay_level_manager.hpp>
#include <okay/core/asset/mesh/mesh_loader.hpp>
#include <okay/core/asset/generic/font_loader.hpp>
#include <okay/core/asset/generic/texture_loader.hpp>
#include <okay/core/asset/okay_asset.hpp>
#include <okay/core/logging/okay_logger.hpp>
#include <okay/core/okay_renderer.hpp>

#include <utility>
#include <okay/core/renderer/okay_material.hpp>
#include <okay/core/renderer/okay_texture.hpp>
#include <okay/core/renderer/okay_render_pipeline.hpp>
#include <okay/core/renderer/okay_uniform.hpp>
#include <okay/core/renderer/passes/scene_pass.hpp>
#include <glm/ext/quaternion_trigonometric.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/gtc/random.hpp>
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/vector_float4.hpp"
#include "okay/core/renderer/materials/lit.hpp"
#include "okay/core/renderer/materials/unlit.hpp"
#include "okay/core/renderer/okay_font.hpp"
#include "okay/core/renderer/okay_mesh.hpp"
#include "okay/core/util/result.hpp"

static void __gameInitialize();
static void __gameUpdate();
static void __gameShutdown();

static okay::OkayRenderEntity g_sun;
static okay::OkayRenderEntity g_planet;
static okay::OkayRenderEntity g_moon;
static okay::OkayRenderEntity g_debugSphere;
static std::size_t g_pointLight;

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

    okay::OkayAssetManager* assetManager = okay::Engine.systems.getSystemChecked<okay::OkayAssetManager>();
    auto teapotRes = assetManager->loadEngineAssetSync<okay::OkayMeshData>("models/teapot.obj");
    if (teapotRes.isError()) {
        okay::Engine.logger.error("Failed to load mesh: {}", teapotRes.error());
        return;
    }

    okay::OkayTextureLoadSettings textureLoadSettings{
        .store = okay::OkayTextureDataStore::mainStore()
    };

    okay::OkayTexture texture = 
        assetManager->loadEngineAssetSync<okay::OkayTexture>("textures/uv_test.jpg", textureLoadSettings).value().asset;

    okay::OkayMesh teapot =
        renderer->meshBuffer().addMesh(
            teapotRes.value().asset
        );

    okay::OkayFontLoadOptions fontLoadOptions;
    fontLoadOptions.height = 32;
    okay::OkayTextOptions textOpt {
        .font = assetManager->loadEngineAssetSync<okay::OkayFontManager::FontHandle>("fonts/ARIAL.TTF", fontLoadOptions).value().asset,
        .meshBuffer = renderer->meshBuffer(),
        .fontSize = 16.0f,
        .verticalSpacing = 4.0f,
        .horizontalAlignment = okay::OkayTextOptions::HoriztonalAlignment::CENTER,
        .verticalAlignment = okay::OkayTextOptions::VerticalAlignment::MIDDLE,
        .doubleSided = true
    };

    okay::OkayMesh textMesh = okay::OkayText::generateTextMesh("hello\nworlkd", textOpt);
    okay::OkayTexture textTexture = okay::OkayFontManager::instance().getGlyphAtlas(textOpt.font);

    okay::OkayMesh cube = 
        renderer->meshBuffer().addMesh(
            okay::primitives::box()
            .sizeSet({0.3f, 0.3f, 0.3f})
            .build(okay::primitives::colorTransform<okay::primitives::BoxBuilder>(glm::vec4(1.0f)))
        );

    okay::Failable res = renderer->meshBuffer().bindMeshData();
    if (res.isError()) {
        okay::Engine.logger.error("Failed to bind mesh data: {}", res.error());
        return;
    }

    auto shaderLoadRes = assetManager->loadEngineAssetSync<okay::OkayShader>("shaders/lit");

    if (shaderLoadRes.isError()) {
        okay::Engine.logger.error("Failed to load shader: {}", shaderLoadRes.error());
        return;
    }

    okay::OkayShaderHandle shader = renderer->materialRegistry().registerShader(shaderLoadRes.value().asset.vertexShader, shaderLoadRes.value().asset.fragmentShader);

    auto materialProprties = std::make_unique<okay::LitMaterial>();
    materialProprties->color.set(glm::vec4(1.0f, 1.0f, 0.0f, 1.0f));
    materialProprties->albedo = textTexture; 
    okay::OkayMaterialHandle sunMat = renderer->materialRegistry().registerMaterial(shader, std::move(materialProprties));
    g_sun = renderer->world().addRenderEntity(
        okay::OkayTransform({0.0f, 0.0f, 0.0f}, {0.05f, 0.05f, 0.05f}),
        sunMat, textMesh
    );

    // planet material
    materialProprties = std::make_unique<okay::LitMaterial>();
    materialProprties->color.set(glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));
    okay::OkayTexture albedo = materialProprties->albedo.edit();
    albedo = texture;
    okay::OkayMaterialHandle planetMat = renderer->materialRegistry().registerMaterial(shader, std::move(materialProprties));
    g_planet = renderer->world().addRenderEntity(
        okay::OkayTransform({ -40.0f, 0.0f, 0.0f }, { 0.3f, 0.3f, 0.3f }),
        planetMat, teapot
    );

    // moon material
    materialProprties = std::make_unique<okay::LitMaterial>();
    materialProprties->color.set(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
    materialProprties->albedo = texture;
    okay::OkayMaterialHandle moonMat = renderer->materialRegistry().registerMaterial(shader, std::move(materialProprties));
    g_moon = renderer->world().addRenderEntity(
        okay::OkayTransform({ 30.0f, 0.0f, 0.0f } , { 0.2f, 0.2f, 0.2f }),
        moonMat, teapot
    );

    g_debugSphere = renderer->world().addRenderEntity(
        okay::OkayTransform({ 0.0f, 0.0f, 0.0f }, { 0.005f, 0.005f, 0.005f }),
        moonMat, teapot
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
            sunMat, cube
        );  
    }

    // add a directional light
    renderer->world().addLight(
        okay::OkayLight::directional(
            glm::vec3(-0.5f, -1, 0.5f), 
            glm::vec3(1.0, 0.8745f, 0.1333f),
            0.6f
        )
    );

    // add a blue point light
    g_pointLight =renderer->world().addLight(
        okay::OkayLight::point(
            glm::vec3(0, 0, 0), 
            3.0f,
            glm::vec3(0.0f, 0.0f, 1.0f),
            20.0f
        )
    );
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
    
    float t = okay::Engine.time->timeSinceStartSec();
    float angle = t * 2.0f;      // radians/sec, tweak
    float r = 0.6f;

    glm::vec3 center = g_sun->transform.position;
    glm::vec3 lightPos = glm::vec3(
        std::sin(angle) * r,
        0.0f,
        std::cos(angle) * r
    );

    renderer->world().getLight(g_pointLight).setPosition(lightPos);
    renderer->world().getLight(g_pointLight).color = glm::vec4(abs(sin(angle * 3.0f)), 0.0f, 1.0f, 1.0f);
    g_debugSphere->transform.position = lightPos;
}

static void __gameShutdown() {
    // Cleanup logic before game shutdown
    okay::Engine.logger.info("Game shutdown.");
}
