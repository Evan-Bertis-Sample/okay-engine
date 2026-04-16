#include "okay/core/ecs/ecs_util.hpp"
#include "okay/core/renderer/uniform.hpp"

#include <okay/okay.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/random.hpp>
#include <utility>

static void __gameInitialize();
static void __gameUpdate();
static void __gameShutdown();

static okay::ECSEntity s_teapot;
static okay::ECSEntity s_light;
static okay::ECSEntity s_camera;

// struct BobComponent {
//    public:
//     std::shared_ptr<okay::Tween<glm::vec3>> positionTween;
// };

// class BobSystem : public okay::ECSSystem<okay::query::Get<okay::TransformComponent, BobComponent>> {
//    public:
//     void onEntityAdded(QueryT::Item& item) override {
//         auto& [transform, bob] = item.components;

//         glm::vec3 offset = glm::ballRand(10.0f);

//         okay::TweenConfig<glm::vec3> config = {
//             .start = transform->position,
//             .end = transform->position + offset,
//             .durationMs = 1000,
//             .easingFn = okay::easing::cubicInOut,
//             .numLoops = -1,
//             .inOutBack = true,
//             .prefixMs = glm::linearRand(0, 1000),
//         };

//         bob.positionTween = okay::Tween<glm::vec3>::create(config);
//         bob.positionTween->start();
//     }

//     void onTick(QueryT::Item& item) override {
//         auto& [transform, bob] = item.components;
//         transform->position = bob.positionTween->value();
//     }

//     void onEntityRemoved(QueryT::Item& item) override {
//         auto& [transform, bob] = item.components;
//         if (bob.positionTween) {
//             bob.positionTween->kill();
//             bob.positionTween = nullptr;
//         }
//     }
// };

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
    okay::Texture texture = okay::load::engineTexture("textures/uv_test.jpg");
    okay::Mesh teapot = okay::mesh(okay::load::engineMeshData("models/teapot.obj"));

    okay::Mesh cube = okay::mesh(okay::primitives::box().build());
    okay::ShaderHandle shader = okay::shaderHandle(okay::load::engineShader("shaders/lit"));

    // Each row varies one Disney BSDF parameter from 0→1 across 11 columns
    using ParamSetter = std::function<void(okay::LitMaterial&, float)>;
    std::vector<ParamSetter> rowParams = {
        [](okay::LitMaterial& m, float v) { m.metallic.set(v); },
        [](okay::LitMaterial& m, float v) { m.specular.set(v); },
        [](okay::LitMaterial& m, float v) { m.specularTint.set(v); },
        [](okay::LitMaterial& m, float v) { m.roughness.set(v); },
        [](okay::LitMaterial& m, float v) { m.anisotropic.set(v); },
        [](okay::LitMaterial& m, float v) { m.sheen.set(v); },
        [](okay::LitMaterial& m, float v) { m.sheenTint.set(v); },
        [](okay::LitMaterial& m, float v) { m.clearcoat.set(v); },
        [](okay::LitMaterial& m, float v) { m.clearcoatGloss.set(v); },
        [](okay::LitMaterial& m, float v) { m.anisotropic.set(v); },
        [](okay::LitMaterial& m, float v) { m.specularTrans.set(v); },
    };

    std::vector<std::vector<okay::MaterialHandle>> materials(11, std::vector<okay::MaterialHandle>(11));
    for (std::size_t i = 0; i < 11; ++i) {
        for (std::size_t j = 0; j < 11; ++j) {
            float t = static_cast<float>(j) / 10.0f;
            auto props = std::make_unique<okay::LitMaterial>();
            props->color.set(glm::vec3(1.0f, 1.0f, 0.0f));
            props->albedo = texture;
            rowParams[i](*props, t);
            materials[i][j] = okay::materialHandle(shader, std::move(props));
        }
    }

    okay::ecs::registerBuiltins();

    s_light = okay::ecs::entity()
                  .addComponent<okay::TransformComponent>(
                      glm::vec3{},
                      glm::vec3{0.1f},
                      glm::angleAxis(glm::radians(90.0f), glm::vec3{0.0f, 1.0f, -5.0f}))
                  .addComponent<okay::LightComponent>(
                      okay::LightComponent::directional(glm::vec3{0.9, 0.9, 0.85}));

    s_camera = okay::ecs::entity()
                   .addComponent<okay::TransformComponent>(glm::vec3{0.0f, 0.0f, 5.0f})
                   .addComponent<okay::CameraComponent>(
                       okay::CameraComponent{okay::Camera::PerspectiveLens{45.0f, 0.1f, 100.0f}});

    for (float i = 0.0f; i < 11.0f; ++i) {
        for (float j = 0.0f; j < 11.0f; ++j) {
            glm::vec3 pos = glm::vec3(-2.5f + j * 0.5f, 3.0f - i * 0.5f, -0.5f);
            okay::ecs::entity()
                .addComponent<okay::TransformComponent>(pos, glm::vec3{0.01f})
                .addComponent<okay::MeshRendererComponent>(teapot, materials[i][j]);
        }
    }

}

static void __gameUpdate() {
    // move the camera in a circle, always looking at the origin
    // float theta = okay::Engine.time->timeSinceStartSec() * 0.05f * glm::pi<float>();
    float theta = 0.0f;
    glm::vec3 pos = glm::vec3(sin(theta) * 5.0f, 0.0f, cos(theta) * 5.0f);
    // rotation much look at origin
    auto& cameraTransform = s_camera.getComponent<okay::TransformComponent>().value();
    cameraTransform->position = pos;
    cameraTransform.lookAt(s_camera, glm::vec3{});

    okay::ECSEntity toDelete;

    // for (auto entity :
    //      okay::ecs::query<okay::query::Get<okay::TransformComponent, BobComponent>>()) {
    //     toDelete = entity.entity;
    //     break;
    // }

    if (toDelete.isValid()) {
        okay::Engine.logger.info("FPS: {} | Destroying entity {}, now {} entities",
                                 okay::Engine.time->fps(),
                                 toDelete.id(),
                                 okay::ecs::entityCount());
        toDelete.destroy();
    }
}

static void __gameShutdown() {
    // Cleanup logic before game shutdown
    okay::Engine.logger.info("Game shutdown.");
}
