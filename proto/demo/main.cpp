#include "okay/core/asset/asset_util.hpp"
#include "okay/core/ecs/components/render_component.hpp"
#include "okay/core/ecs/components/transform_component.hpp"
#include "okay/core/ecs/ecstore.hpp"
#include "okay/core/renderer/material.hpp"
#include "okay/core/renderer/materials/lit.hpp"
#include "okay/core/renderer/mesh.hpp"
#include "okay/core/renderer/renderer_util.hpp"
#include "okay/core/renderer/texture.hpp"
#include <okay/okay.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/random.hpp>
#include <imgui.h>
#include <memory>
#include <utility>

namespace ui = okay::ui;

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

    okay::RendererSettings rendererSettings{.surfaceConfig = surfaceConfig,
        .pipeline = okay::RenderPipeline::create(std::make_unique<okay::ScenePass>()),
        .enableIMGUI = false};

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
    // okay::Texture tex2 = okay::load::engineTexture("textures/nupurple.jpg");
    // okay::Texture woodTex = okay::load::engineTexture("textures/woodfloor.jpg");
    // okay::Texture yellowTex = okay::load::engineTexture("textures/yellow.jpg");

    okay::Mesh object = okay::mesh(okay::load::engineMeshData("models/dragon.obj"));
    // okay::Mesh mitsubaMesh = okay::mesh(okay::load::engineMeshData("models/mitsuba-sphere.obj"));
    // okay::Mesh carMesh = okay::mesh(okay::load::engineMeshData("models/sportsCar.obj"));
    // okay::Mesh bunnyMesh = okay::mesh(okay::load::engineMeshData("models/bunny.obj"));

    okay::Mesh cube = okay::mesh(okay::primitives::box().build());
    okay::ShaderHandle shader = okay::shaderHandle(okay::load::engineShader("shaders/lit"));

    auto materialProperties = std::make_unique<okay::LitMaterial>();
    materialProperties->color.set(glm::vec4(0.8f, 0.2f, 0.3f, 1.0f));
    materialProperties->albedo = texture;
    // materialProperties->clearcoat.set(0.75f);
    materialProperties->roughness.set(0.75f);
    materialProperties->sheen.set(1.0f);
    // materialProperties->sheenTint.set(1.0f);
    // materialProperties->isTransparent = true;
    // materialProperties->opacity.set(0.5f);
    // materialProperties->thin.set(1);
    // materialProperties->specularTrans.set(0.5f);
    // materialProperties->thin.set(1);
    // materialProperties->specularTint.set(0.5f);
    okay::MaterialHandle material = okay::materialHandle(shader, std::move(materialProperties));

    // auto mat2Props = std::make_unique<okay::LitMaterial>();
    // mat2Props->color.set(glm::vec4(0.4f, 0.1f, 0.8f, 1.0f));
    // mat2Props->albedo = tex2;
    // okay::MaterialHandle mat2 = okay::materialHandle(shader, std::move(mat2Props));

    // auto tableProps = std::make_unique<okay::LitMaterial>();
    // // tableProps->color.set(glm::vec4(0.8f, 0.6f, 0.35f, 1.0f));
    // tableProps->albedo = woodTex;
    // tableProps->clearcoat.set(1.0f);
    // tableProps->specular.set(0.5f);
    // okay::MaterialHandle tableMat = okay::materialHandle(shader, std::move(tableProps));

    // auto mitsubaProps = std::make_unique<okay::LitMaterial>();
    // mitsubaProps->color.set(glm::vec4(1.0f, 0.87f, 0.0f, 1.0f));
    // mitsubaProps->albedo = yellowTex;
    // mitsubaProps->metallic.set(1.0f);
    // mitsubaProps->anisotropic.set(0.5f);
    // okay::MaterialHandle mitsubaMat = okay::materialHandle(shader, std::move(mitsubaProps));

    // auto carProps = std::make_unique<okay::LitMaterial>();
    // carProps->color.set(glm::vec4(0.306f, 0.165f, 0.518f, 1.0f));
    // // carProps->albedo = tex2;
    // carProps->metallic.set(0.75f);
    // carProps->specular.set(0.5f);
    // carProps->clearcoat.set(0.75f);
    // okay::MaterialHandle carMat = okay::materialHandle(shader, std::move(carProps));

    // auto bunnyProps = std::make_unique<okay::LitMaterial>();
    // bunnyProps->color.set(glm::vec4(0.4f, 0.8f, 0.2f, 1.0f));
    // bunnyProps->roughness.set(0.8f);
    // okay::MaterialHandle bunnyMat = okay::materialHandle(shader, std::move(bunnyProps));

    okay::ecs::registerBuiltins();

    s_light = okay::ecs::entity()
                  .addComponent<okay::TransformComponent>(glm::vec3{},
                      glm::vec3{0.1f},
                      glm::angleAxis(glm::radians(0.0f), glm::vec3{1.0f, 0.0f, 0.0f}))
                  .addComponent<okay::LightComponent>(
                      okay::LightComponent::directional(glm::vec3{1, 1, 1}, 4.0f));

    s_camera = okay::ecs::entity()
                   .addComponent<okay::TransformComponent>(glm::vec3{0.0f, 0.0f, 1.0f})
                   .addComponent<okay::CameraComponent>(
                       okay::CameraComponent{okay::Camera::PerspectiveLens{45.0f, 0.1f, 100.0f}});

    s_teapot = okay::ecs::entity()
                   .addComponent<okay::TransformComponent>(glm::vec3{}, glm::vec3{3.0f}, glm::angleAxis(glm::radians(120.0f), glm::vec3{0.0f, 1.0f, 0.0f}))
                   .addComponent<okay::MeshRendererComponent>(object, material);


    // okay::ECSEntity table = okay::ecs::entity()
    //                .addComponent<okay::TransformComponent>(glm::vec3{0.0f, -1.0f, 0.0f}, glm::vec3{7.0f, 0.3f, 7.0f})
    //                .addComponent<okay::MeshRendererComponent>(cube, tableMat);

    // okay::ECSEntity mitsuba = okay::ecs::entity()
    //                .addComponent<okay::TransformComponent>(glm::vec3{1.7f, -0.75f, 0.5f}, glm::vec3{0.4f})
    //                .addComponent<okay::MeshRendererComponent>(mitsubaMesh, mitsubaMat);

    // okay::ECSEntity car = okay::ecs::entity()
    //                .addComponent<okay::TransformComponent>(glm::vec3{0.0f, -0.8f, 2.0f}, glm::vec3{0.3f}, glm::angleAxis(glm::radians(-25.0f), glm::vec3{0.0f, 1.0f, 0.0f}))
    //                .addComponent<okay::MeshRendererComponent>(carMesh, carMat);

    // okay::ECSEntity bunny = okay::ecs::entity()
    //                .addComponent<okay::TransformComponent>(glm::vec3{-1.5f, -0.75f, 1.0f}, glm::vec3{0.75f})
    //                .addComponent<okay::MeshRendererComponent>(bunnyMesh, bunnyMat);

    // for (std::size_t i = 0; i < 1000; ++i) {
    //     glm::vec3 pos = glm::ballRand(50.0f);
    //     okay::ECSEntity entity = okay::ecs::entity()
    //                                  .addComponent<okay::TransformComponent>(pos, glm::vec3{0.5f})
    //                                  .addComponent<okay::MeshRendererComponent>(cube, material);

    //     for (std::size_t i = 0; i < 5; ++i) {
    //         pos = glm::ballRand(10.0f);
    //         okay::ecs::entity(entity)
    //             .addComponent<okay::TransformComponent>(pos, glm::vec3{0.5f})
    //             .addComponent<okay::MeshRendererComponent>(cube, material);
    //     }
    // }

    // okay::ecs::entity().addComponent<okay::TransformComponent>().addComponent<okay::UIComponent>(
    //     []() {
    //         return ui::frame(10, 10, 200, 100)(ui::flexbox()
    //                 .marginSet(10)
    //                 .paddingSet(10)
    //                 .rightPaddingSet(20)
    //                 .backgroundColorSet(glm::vec4{0.05f, 0.0f, 0.05f, 0.5f})
    //                 .borderColorSet(glm::vec4{1.0f, 1.0f, 1.0f, 0.8f})
    //                 .borderRadiusSet(5)
    //                 .borderWidthSet(1)(ui::h3("Performance"),
    //                     ui::vspacer(10),
    //                     ui::h3(std::format("FPS: {:2f}", okay::Engine.time->fps())),
    //                     ui::h2(std::format("Entity count: {}", okay::ecs::entityCount()))));
    //     });
}

static void __gameUpdate() {
    // move the camera in a circle, always looking at the origin
    // float theta = okay::Engine.time->timeSinceStartSec() * 0.5f * glm::pi<float>();
    float theta = 180.0f;
    const float distance = 5.0f;
    glm::vec3 pos = glm::vec3(sin(theta) * distance, 0.0f, cos(theta) * distance);
    // glm::vec3 pos = {0.0f, 0.5f, 5.0f};
    // rotation much look at origin
    auto& cameraTransform = s_camera.getComponent<okay::TransformComponent>().value();
    cameraTransform->position = pos;
    cameraTransform.lookAt(s_camera, glm::vec3{});
}

static void __gameShutdown() {
    // Cleanup logic before game shutdown
    okay::Engine.logger.info("Game shutdown.");
}
