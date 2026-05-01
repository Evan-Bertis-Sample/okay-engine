
#include <okay/okay.hpp>

#include <glm/glm.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/gtc/random.hpp>

#include <utility>

static void __gameInitialize();
static void __gameUpdate();
static void __gameShutdown();

static okay::ECSEntity s_teapot;
static okay::ECSEntity s_light;
static okay::ECSEntity s_camera;

int main() {
    okay::SurfaceConfig surfaceConfig;
    surfaceConfig.width = 800;
    surfaceConfig.height = 480;
    okay::Surface surface(surfaceConfig);

    okay::RendererSettings rendererSettings{
        .surfaceConfig = surfaceConfig,
        .pipeline = okay::RenderPipeline::create(std::make_unique<okay::ScenePass>())};

    auto renderer = okay::Renderer::create(std::move(rendererSettings));

    okay::Game::create()
        .addSystems(std::move(renderer),
                    std::make_unique<okay::AssetManager>(),
                    std::make_unique<okay::ECS>(),
                    std::make_unique<okay::TweenEngine>())
        .onInitialize(__gameInitialize)
        .onUpdate(__gameUpdate)
        .onShutdown(__gameShutdown)
        .run();

    return 0;
}

static void __gameInitialize() {
    // Additional game initialization logic
    okay::Renderer* renderer = okay::Engine.systems.getSystemChecked<okay::Renderer>();
    okay::AssetManager* assetManager = okay::Engine.systems.getSystemChecked<okay::AssetManager>();
    auto teapotRes = assetManager->loadEngineAssetSync<okay::MeshData>("models/teapot.obj");
    if (teapotRes.isError()) {
        okay::Engine.logger.error("Failed to load mesh: {}", teapotRes.error());
        return;
    }

    okay::TextureLoadSettings textureLoadSettings(okay::TextureDataStore::mainStore());
    okay::Texture texture =
        assetManager
            ->loadEngineAssetSync<okay::Texture>("textures/uv_test.jpg", textureLoadSettings)
            .value()
            .asset;

    okay::FontLoadOptions fontLoad = {
        .width = 32,
    };

    okay::FontManager::FontHandle font = assetManager->loadEngineAssetSync<okay::FontManager::FontHandle>(
        "fonts/ARIAL.TTF", fontLoad).value().asset;

    okay::Mesh teapot = renderer->meshBuffer().addMesh(teapotRes.value().asset);
    okay::Mesh cube = renderer->meshBuffer().addMesh(okay::primitives::box().build());

    okay::TextOptions textOptions = {
        .font = font,
        .meshBuffer = renderer->meshBuffer(),
        .fontSize = 1.0f,
        .horizontalAlignment = okay::TextOptions::HoriztonalAlignment::CENTER,
    };

    okay::Mesh textMesh = okay::OkayText::generateTextMesh("Hello world!", textOptions);

    okay::Failable res = renderer->meshBuffer().bindMeshData();
    if (res.isError()) {
        okay::Engine.logger.error("Failed to bind mesh data: {}", res.error());
        return;
    }

    auto shaderLoadRes = assetManager->loadEngineAssetSync<okay::Shader>("shaders/text_sdf");
    if (shaderLoadRes.isError()) {
        okay::Engine.logger.error("Failed to load shader: {}", shaderLoadRes.error());
        return;
    }

    okay::ShaderHandle shader = renderer->materialRegistry().registerShader(
        shaderLoadRes.value().asset.vertexShader, shaderLoadRes.value().asset.fragmentShader);
    auto uvMaterial = std::make_unique<okay::LitMaterial>();
    uvMaterial->color.set(glm::vec4(1.0f, 1.0f, 0.0f, 1.0f));
    uvMaterial->albedo = texture;

    okay::MaterialHandle uvTestMaterial =
        renderer->materialRegistry().registerMaterial(shader, std::move(uvMaterial));

    auto textMaterial = std::make_unique<okay::LitMaterial>();
    textMaterial->color.set(glm::vec4(1.0f, 1.0f, 0.0f, 1.0f));
    textMaterial->albedo = okay::FontManager::instance().getGlyphAtlas(font);

    okay::MaterialHandle textMaterialHandle =
        renderer->materialRegistry().registerMaterial(shader, std::move(textMaterial));

    okay::ECS* ecs = okay::Engine.systems.getSystemChecked<okay::ECS>();
    okay::registerBuiltinComponentsAndSystems(ecs);
    s_teapot =
        ecs->createEntity()
            .addComponent<okay::TransformComponent>(glm::vec3{0.0f, 0.0f, 0.0f}, glm::vec3{0.1f})
            .addComponent<okay::MeshRendererComponent>(teapot, uvTestMaterial);

    s_light = ecs->createEntity()
                  .addComponent<okay::TransformComponent>(
                      glm::vec3{},
                      glm::vec3{0.1f},
                      glm::angleAxis(glm::radians(45.0f), glm::vec3{0.0f, 1.0f, 0.0f}))
                  .addComponent<okay::LightComponent>(
                      okay::LightComponent::directional(glm::vec3{0.9, 0.9, 0.85}));

    s_camera = ecs->createEntity()
                   .addComponent<okay::TransformComponent>(glm::vec3{0.0f, 0.0f, 5.0f})
                   .addComponent<okay::CameraComponent>(
                       okay::CameraComponent{okay::Camera::PerspectiveLens{45.0f, 0.1f, 100.0f}});

    // text
    ecs->createEntity()
        .addComponent<okay::TransformComponent>(glm::vec3{0.0f, 0.5f, 0.0f}, glm::vec3{1.0f})
        .addComponent<okay::MeshRendererComponent>(textMesh, textMaterialHandle);

}

static void __gameUpdate() {
    // move the camera in a circle, always looking at the origin
    float theta = okay::Engine.time->timeSinceStartSec() * 0.05f * glm::pi<float>();
    glm::vec3 pos = glm::vec3(sin(theta) * 5.0f, 0.0f, cos(theta) * 5.0f);
    // rotation much look at origin
    auto& cameraTransform = s_camera.getComponent<okay::TransformComponent>().value();
    cameraTransform->position = pos;
    cameraTransform.lookAt(s_camera, glm::vec3{});
}

static void __gameShutdown() {
    // Cleanup logic before game shutdown
    okay::Engine.logger.info("Game shutdown.");
}
