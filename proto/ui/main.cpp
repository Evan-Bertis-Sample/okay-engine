#include <okay/okay.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/random.hpp>
#include <thread>
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
    okay::Texture texture = okay::load::engineTexture("textures/uv_test.jpg");
    okay::Mesh teapot = okay::mesh(okay::load::engineMeshData("models/teapot.obj"));

    okay::Mesh cube = okay::mesh(okay::primitives::box().build());
    okay::ShaderHandle shader = okay::shaderHandle(okay::load::engineShader("shaders/lit"));

    auto materialProperties = std::make_unique<okay::LitMaterial>();
    materialProperties->color.set(glm::vec4(1.0f, 1.0f, 0.0f, 1.0f));
    materialProperties->albedo = texture;
    okay::MaterialHandle material = okay::materialHandle(shader, std::move(materialProperties));

    okay::ecs::registerBuiltins();
    s_teapot =
        okay::ecs::entity()
            .addComponent<okay::TransformComponent>(glm::vec3{0.0f, 0.0f, 0.0f}, glm::vec3{0.1f})
            .addComponent<okay::MeshRendererComponent>(teapot, material);

    s_light = okay::ecs::entity()
                  .addComponent<okay::TransformComponent>(
                      glm::vec3{},
                      glm::vec3{0.1f},
                      glm::angleAxis(glm::radians(45.0f), glm::vec3{0.0f, 1.0f, 0.0f}))
                  .addComponent<okay::LightComponent>(
                      okay::LightComponent::directional(glm::vec3{0.9, 0.9, 0.85}));

    s_camera = okay::ecs::entity()
                   .addComponent<okay::TransformComponent>(glm::vec3{0.0f, 0.0f, 5.0f})
                   .addComponent<okay::CameraComponent>(
                       okay::CameraComponent{okay::Camera::PerspectiveLens{45.0f, 0.1f, 100.0f}});

    // clang-format off
    okay::ecs::entity()
        .addComponent<okay::TransformComponent>(glm::vec3{0.0f, 0.0f, 0.0f})
        .addComponent<okay::UIComponent>(
            okay::ui::flexbox()(
                okay::ui::flexbox()
                    .backgroundColorSet(glm::vec4{1.0f, 1.0f, 1.0f, 1.0f})
                    .backgroundImageSet(okay::load::engineTexture("textures/uv_test.jpg"))
                    (
                        okay::ui::text("Hello world!")
                            .textColorSet(glm::vec3(0.0f, 0.0f, 0.0f))
                            .widthGrow()
                            .heightGrow()
                            .backgroundColorSet(glm::vec4(1.0f, 0.0f, 1.0f, 1.0f))
                            .alignTextHorizontal(okay::TextStyle::HorizontalAlignment::Center)
                            .alignTextVertical(okay::TextStyle::VerticalAlignment::Middle)
                            .paddingSet(10)
                        ,
                        okay::ui::growBox(okay::UIPrimaryAxis::Horizontal)
                        (
                            okay::ui::text("Left")
                                .textColorSet(glm::vec3(1.0f, 0.0f, 1.0f))
                                .backgroundColorSet(glm::vec4(0.0f, 1.0f, 0.0f, 1.0f))
                                .alignTextCenter()
                                .paddingSet(10)
                                .widthGrow()
                            ,
                            okay::ui::growBox(okay::UIPrimaryAxis::Vertical)
                                .backgroundColorSet(glm::vec4(0.0f, 0.0f, 1.0f, 1.0f))
                                .borderRadiusSet(30)
                                .borderWidthSet(10)
                            (
                                okay::ui::growBox(okay::UIPrimaryAxis::Vertical) 
                                    .childSpacingSet(10)
                                (
                                    okay::ui::text("Right")
                                        .textColorSet(glm::vec3(0.0f, 0.0f, 0.0f))
                                        .alignTextRight()
                                        .alignTextCenter()
                                        .grow()
                                    ,
                                    okay::ui::text("top")
                                    ,
                                    okay::ui::range(
                                        3,
                                        [](std::int32_t i) {
                                            return okay::ui::text(
                                                std::format("{}", i));
                                        }
                                    )
                                    ,
                                    okay::ui::text("bottom")   
                                )
                                
                            )
                        )
                    )
            )
        );
    //clang-format on
}

static void __gameUpdate() {
}

static void __gameShutdown() {
    // Cleanup logic before game shutdown
    okay::Engine.logger.info("Game shutdown.");
}
