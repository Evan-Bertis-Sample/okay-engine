#include "glm/gtc/quaternion.hpp"
#include "okay/core/ecs/ecs_util.hpp"
#include "okay/core/renderer/material.hpp"
#include "okay/core/renderer/materials/lit.hpp"
#include "okay/core/renderer/uniform.hpp"

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
    okay::Texture texture = okay::load::engineTexture("textures/uv_test.jpg");
    okay::Texture floorTex = okay::load::engineTexture("textures/WoodFlooring.jpg");
    okay::Mesh object = okay::mesh(okay::load::engineMeshData("models/teapot.obj"));

    okay::Mesh floor = okay::mesh(
        okay::primitives::box()
        .sizeSet(glm::vec3(10.0f, 0.3f, 10.0f))
        .build());

    okay::Mesh cube = okay::mesh(okay::primitives::box().build());
    okay::ShaderHandle shader = okay::shaderHandle(okay::load::engineShader("shaders/lit"));

    auto props = std::make_unique<okay::LitMaterial>();
    props->color.set(glm::vec3(0.8, 0.8, 0.8)); 
    props->albedo = texture;
    props->roughness.set(0.75f);
    // props->metallic.set(0.75f);
    props->clearcoat.set(0.75f);
    okay::MaterialHandle material = okay::materialHandle(shader, std::move(props));

    auto floorProps = std::make_unique<okay::LitMaterial>();
    floorProps->albedo = floorTex;
    floorProps->roughness.set(0.75f);
    okay::MaterialHandle floorMat = okay::materialHandle(shader, std::move(floorProps));

    okay::ecs::registerBuiltins();

    glm::vec3 ldirection = glm::normalize(glm::vec3(-3.0f, -2.0f, 0.0f));
    glm::quat rotation = glm::quatLookAt(ldirection, glm::vec3(0.0f, 1.0f, 0.0f));

    s_light = okay::ecs::entity()
                  .addComponent<okay::TransformComponent>(
                      glm::vec3{6.0f, 4.0f, 0.0f},
                      glm::vec3{1.0f},
                      rotation)
                  .addComponent<okay::LightComponent>(
                      okay::LightComponent::directional(glm::vec3{1, 1, 1}, 2.5f));

    s_camera = okay::ecs::entity()
                   .addComponent<okay::TransformComponent>(glm::vec3{0.0f, 0.0f, 0.0f})
                   .addComponent<okay::CameraComponent>(
                       okay::CameraComponent{okay::Camera::PerspectiveLens{45.0f, 0.1f, 100.0f}});

    glm::vec3 pos1 = glm::vec3(0.0f, 0.0f, 2.0f);
    okay::ecs::entity()
        .addComponent<okay::TransformComponent>(
            pos1,
            glm::vec3{0.1f},
            glm::angleAxis(glm::radians(0.0f), glm::vec3{0.0f, 1.0f, 0.0f}))
        .addComponent<okay::MeshRendererComponent>(object, material);

    glm::vec3 pos2 = glm::vec3(3.0f, 1.0f, 1.0f);
    okay::ecs::entity()
        .addComponent<okay::TransformComponent>(
            pos2,
            glm::vec3{0.1f},
            glm::angleAxis(glm::radians(0.0f), glm::vec3{0.0f, 1.0f, 0.0f}))
        .addComponent<okay::MeshRendererComponent>(object, material);

    okay::ecs::entity()
        .addComponent<okay::TransformComponent>(
            glm::vec3(0.0, -2.0, 0.0),
            glm::vec3{3.0f},
            glm::angleAxis(glm::radians(0.0f), glm::vec3{0.0f, 1.0f, 0.0f}))
        .addComponent<okay::MeshRendererComponent>(floor, floorMat);

    

}

static void __gameUpdate() {
    // move the camera in a circle, always looking at the origin
    float theta = okay::Engine.time->timeSinceStartSec() * 0.05f * glm::pi<float>();
    // float theta = 0.0f;
    glm::vec3 pos = glm::vec3(sin(theta) * 10.0f, 2.0f, cos(theta) * 10.0f);
    // glm::vec3 pos = glm::vec3(5.0, 5.0, -10.0);
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
    // ImGui::ShowDemoWindow();
}

static void __gameShutdown() {
    // Cleanup logic before game shutdown
    okay::Engine.logger.info("Game shutdown.");
}
