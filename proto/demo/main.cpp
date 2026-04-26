#include <okay/okay.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/random.hpp>
#include <imgui.h>
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
        .pipeline = okay::RenderPipeline::create(std::make_unique<okay::ScenePass>()),
        .enableIMGUI = true};

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
    okay::Texture texture = okay::load::engineTexture("textures/red.jpg");
    okay::Mesh object = okay::mesh(okay::load::engineMeshData("models/dragon.obj"));

    okay::Mesh cube = okay::mesh(okay::primitives::box().build());
    okay::ShaderHandle shader = okay::shaderHandle(okay::load::engineShader("shaders/lit"));

    auto materialProperties = std::make_unique<okay::LitMaterial>();
    materialProperties->color.set(glm::vec4(1.0f, 1.0f, 0.0f, 1.0f));
    materialProperties->albedo = texture;
    okay::MaterialHandle material = okay::materialHandle(shader, std::move(materialProperties));

    okay::FontManager::FontHandle font = okay::load::engineFont("fonts/ARIAL.TTF");
    okay::TextStyle style{.font = font,
                          .fontHeight = 1.0f,
                          .horizontalAlignment = okay::TextStyle::HorizontalAlignment::Center,
                          .verticalAlignment = okay::TextStyle::VerticalAlignment::Top};

    okay::Mesh textMesh = okay::mesh(okay::textMesh("Hello, world!", style, true));

    auto textProperties = std::make_unique<okay::LitMaterial>();
    textProperties->albedo = okay::FontManager::instance().getGlyphAtlas(font);
    textProperties->isTransparent = true;
    textProperties->color = glm::vec3(1.0);
    okay::MaterialHandle textMaterial = okay::materialHandle(shader, std::move(textProperties));

    okay::ecs::registerBuiltins();

    s_light = okay::ecs::entity()
                  .addComponent<okay::TransformComponent>(
                      glm::vec3{},
                      glm::vec3{0.1f},
                      glm::angleAxis(glm::radians(0.0f), glm::vec3{2.0f, 3.0f, 1.0f}))
                  .addComponent<okay::LightComponent>(
                      okay::LightComponent::directional(glm::vec3{1, 1, 1}, 2.5f));

    s_camera = okay::ecs::entity()
                   .addComponent<okay::TransformComponent>(glm::vec3{0.0f, 0.0f, 5.0f})
                   .addComponent<okay::CameraComponent>(
                       okay::CameraComponent{okay::Camera::PerspectiveLens{45.0f, 0.1f, 100.0f}});

    okay::ecs::entity()
        .addComponent<okay::TransformComponent>(glm::vec3{0.0f, 2.0f, 0.0f})
        .addComponent<okay::MeshRendererComponent>(textMesh, textMaterial);

    for (std::size_t i = 0; i < 1000; ++i) {
        glm::vec3 pos = glm::ballRand(50.0f);
        okay::ECSEntity entity = okay::ecs::entity()
                                     .addComponent<okay::TransformComponent>(pos, glm::vec3{0.5f})
                                     .addComponent<okay::MeshRendererComponent>(cube, material)
                                     .addComponent<BobComponent>();

        for (std::size_t i = 0; i < 5; ++i) {
            pos = glm::ballRand(100.0f);
            okay::ecs::entity(entity)
                .addComponent<okay::TransformComponent>(pos, glm::vec3{0.5f})
                .addComponent<okay::MeshRendererComponent>(cube, material);
        }
    }
}

static void __gameUpdate() {
    // move the camera in a circle, always looking at the origin
    float theta = okay::Engine.time->timeSinceStartSec() * 0.05f * glm::pi<float>();
    const float distance = 10.0f;
    glm::vec3 pos = glm::vec3(sin(theta) * distance, 1.0f, cos(theta) * distance);
    // rotation much look at origin
    auto& cameraTransform = s_camera.getComponent<okay::TransformComponent>().value();
    cameraTransform->position = pos;
    cameraTransform.lookAt(s_camera, glm::vec3{});

    okay::ECSEntity toDelete;

    if (toDelete.isValid()) {
        okay::Engine.logger.info("FPS: {} | Destroying entity {}, now {} entities",
                                 okay::Engine.time->fps(),
                                 toDelete.id(),
                                 okay::ecs::entityCount());
        toDelete.destroy();
    }
    ImGui::ShowDemoWindow();
}

static void __gameShutdown() {
    // Cleanup logic before game shutdown
    okay::Engine.logger.info("Game shutdown.");
}
