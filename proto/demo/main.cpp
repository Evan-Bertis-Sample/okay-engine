#include <okay/okay.hpp>

#include <glm/glm.hpp>
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
                    std::make_unique<okay::ECS>())
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

    okay::TextureLoadSettings textureLoadSettings{.store = okay::TextureDataStore::mainStore()};
    okay::Texture texture =
        assetManager
            ->loadEngineAssetSync<okay::Texture>("textures/uv_test.jpg", textureLoadSettings)
            .value()
            .asset;
    okay::Mesh teapot = renderer->meshBuffer().addMesh(teapotRes.value().asset);

    okay::Failable res = renderer->meshBuffer().bindMeshData();
    if (res.isError()) {
        okay::Engine.logger.error("Failed to bind mesh data: {}", res.error());
        return;
    }

    auto shaderLoadRes = assetManager->loadEngineAssetSync<okay::Shader>("shaders/lit");
    if (shaderLoadRes.isError()) {
        okay::Engine.logger.error("Failed to load shader: {}", shaderLoadRes.error());
        return;
    }

    okay::ShaderHandle shader = renderer->materialRegistry().registerShader(
        shaderLoadRes.value().asset.vertexShader, shaderLoadRes.value().asset.fragmentShader);
    auto materialProperties = std::make_unique<okay::LitMaterial>();
    materialProperties->color.set(glm::vec4(1.0f, 1.0f, 0.0f, 1.0f));
    materialProperties->albedo = texture;

    okay::MaterialHandle material =
        renderer->materialRegistry().registerMaterial(shader, std::move(materialProperties));

    okay::ECS* ecs = okay::Engine.systems.getSystemChecked<okay::ECS>();
    okay::registerBuiltinComponentsAndSystems(*ecs);

    s_teapot =
        ecs->createEntity()
            .addComponent<okay::TransformComponent>(glm::vec3{0.0f, 0.0f, -5.0f}, glm::vec3{0.1f})
            .addComponent<okay::MeshRendererComponent>(teapot, material);

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
}

static void __gameUpdate() {
    // move the camera in a circle, always looking at the origin
    float theta = okay::Engine.time->timeSinceStartSec() * 0.05f * glm::pi<float>();
    glm::vec3 pos = glm::vec3(sin(theta) * 5.0f, 0.0f, cos(theta) * 5.0f);
    // rotation much look at origin
    auto &cameraTransform = s_camera.getComponent<okay::TransformComponent>().value();
    cameraTransform->position = pos;
    cameraTransform.setLocalDirection(-pos);
}

static void __gameShutdown() {
    // Cleanup logic before game shutdown
    okay::Engine.logger.info("Game shutdown.");
}
