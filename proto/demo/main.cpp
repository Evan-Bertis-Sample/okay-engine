
#include "glm/gtc/random.hpp"
#include "okay/core/ecs/components/transform_component.hpp"
#include "okay/core/ecs/ecstore.hpp"
#include "okay/core/engine/engine.hpp"
#include "okay/core/tween/tween_easing.hpp"
#include "okay/core/tween/tween_engine.hpp"

#include <okay/okay.hpp>

#include <glm/glm.hpp>
#include <utility>

static void __gameInitialize();
static void __gameUpdate();
static void __gameShutdown();

static okay::ECSEntity s_teapot;
static okay::ECSEntity s_light;
static okay::ECSEntity s_camera;

struct BobComponent {
   public:
    std::shared_ptr<okay::Tween<glm::vec3>> positionTween;
};

class BobSystem : public okay::ECSSystem<okay::query::Get<okay::TransformComponent, BobComponent>> {
   public:
    void onEntityAdded(QueryT::Item& item) override {
        auto& [transform, bob] = item.components;

        glm::vec3 offset = glm::ballRand(10.0f);

        okay::TweenConfig<glm::vec3> config = {
            .start = transform->position,
            .end = transform->position + offset,
            .durationMs = 1000,
            .easingFn = okay::easing::cubicInOut,
            .numLoops = -1,
            .inOutBack = true,
            .prefixMs = glm::linearRand(0, 1000),
        };

        bob.positionTween = okay::Tween<glm::vec3>::create(config);
        bob.positionTween->start();

        // okay::Engine.logger.debug("BobSystem: Entity {} added", item.entity.id());
    }

    void onTick(QueryT::Item& item) override {
        auto& [transform, bob] = item.components;
        transform->position = bob.positionTween->value();
    }

    void onEntityRemoved(QueryT::Item& item) override {
        auto& [transform, bob] = item.components;
        if (bob.positionTween) {
            bob.positionTween->kill();
            bob.positionTween = nullptr;
        }
    }
};

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

    ecs->registerComponentType<BobComponent>();
    ecs->addSystem(std::make_unique<BobSystem>());

    s_teapot =
        ecs->createEntity()
            .addComponent<okay::TransformComponent>(glm::vec3{0.0f, 0.0f, 0.0f}, glm::vec3{0.1f})
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

    for (std::size_t i = 0; i < 1000; ++i) {
        glm::vec3 pos = glm::ballRand(50.0f);
        okay::ECSEntity entity = ecs->createEntity()
                                     .addComponent<okay::TransformComponent>(pos, glm::vec3{0.05f})
                                     .addComponent<okay::MeshRendererComponent>(teapot, material)
                                     .addComponent<BobComponent>();

        for (std::size_t i = 0; i < 5; ++i) {
            pos = glm::ballRand(100.0f);
            ecs->createEntity(entity)
                .addComponent<okay::TransformComponent>(pos, glm::vec3{0.5f})
                .addComponent<okay::MeshRendererComponent>(teapot, material);
        }
    }
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
