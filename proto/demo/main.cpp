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

static okay::ECSEntity s_teapot1, s_teapot2, s_teapot3, s_teapot4;
static okay::ECSEntity s_light;
static okay::ECSEntity s_camera;
static glm::vec3 s_pos1, s_pos2, s_pos3, s_pos4;


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
    okay::Texture texture = okay::load::engineTexture("textures/red.jpg");
    okay::Texture floorTex = okay::load::engineTexture("textures/WoodFlooring.jpg");
    okay::Mesh object = okay::mesh(okay::load::engineMeshData("models/teapot.obj"));

    okay::Mesh floor = okay::mesh(
        okay::primitives::box()
        .sizeSet(glm::vec3(10.0f, 0.3f, 10.0f))
        .build());

    okay::ShaderHandle shader = okay::shaderHandle(okay::load::engineShader("shaders/lit"));

    auto props = std::make_unique<okay::LitMaterial>();
    props->color.set(glm::vec3(0.8, 0.8, 0.8));
    props->albedo = texture;
    props->roughness.set(0.75f);
    props->clearcoat.set(0.75f);
    okay::MaterialHandle material = okay::materialHandle(shader, std::move(props));

    auto floorProps = std::make_unique<okay::LitMaterial>();
    floorProps->albedo = floorTex;
    floorProps->roughness.set(0.75f);
    floorProps->castsShadows = false;
    okay::MaterialHandle floorMat = okay::materialHandle(shader, std::move(floorProps));

    okay::ecs::registerBuiltins();

    glm::vec3 ldirection = glm::normalize(glm::vec3(-3.0f, -2.0f, 0.0f));
    glm::quat lrotation = glm::quatLookAt(ldirection, glm::vec3(0.0f, 1.0f, 0.0f));
    s_light = okay::ecs::entity()
                  .addComponent<okay::TransformComponent>(glm::vec3{6.0f, 4.0f, 0.0f}, glm::vec3{1.0f}, lrotation)
                  .addComponent<okay::LightComponent>(okay::LightComponent::directional(glm::vec3{1, 1, 1}, 2.5f));

    s_camera = okay::ecs::entity()
                   .addComponent<okay::TransformComponent>(glm::vec3{0.0f, 0.0f, 0.0f})
                   .addComponent<okay::CameraComponent>(
                       okay::CameraComponent{okay::Camera::PerspectiveLens{45.0f, 0.1f, 100.0f}});

    // Teapots — each slides along a different axis, staggered so they don't move in sync
    glm::quat noRot = glm::angleAxis(glm::radians(0.0f), glm::vec3{0.0f, 1.0f, 0.0f});

    s_pos1 = glm::vec3(0.0f, -1.0f, 0.0f);
    s_teapot1 = okay::ecs::entity()
        .addComponent<okay::TransformComponent>(s_pos1, glm::vec3{0.1f}, noRot)
        .addComponent<okay::MeshRendererComponent>(object, material);

    s_pos2 = glm::vec3(3.0f, -1.0f, 0.0f);
    s_teapot2 = okay::ecs::entity()
        .addComponent<okay::TransformComponent>(s_pos2, glm::vec3{0.1f}, noRot)
        .addComponent<okay::MeshRendererComponent>(object, material);

    s_pos3 = glm::vec3(-3.0f, -1.0f, 0.0f);
    s_teapot3 = okay::ecs::entity()
        .addComponent<okay::TransformComponent>(s_pos3, glm::vec3{0.1f}, noRot)
        .addComponent<okay::MeshRendererComponent>(object, material);

    s_pos4 = glm::vec3(0.0f, -1.0f, -3.0f);
    s_teapot4 = okay::ecs::entity()
        .addComponent<okay::TransformComponent>(s_pos4, glm::vec3{0.1f}, noRot)
        .addComponent<okay::MeshRendererComponent>(object, material);

    okay::ecs::entity()
        .addComponent<okay::TransformComponent>(glm::vec3(0.0, -2.0, 0.0), glm::vec3{3.0f}, noRot)
        .addComponent<okay::MeshRendererComponent>(floor, floorMat);

    okay::Tween<glm::vec3>::create(okay::TweenConfig<glm::vec3>{
        .start      = s_pos1,
        .end        = s_pos1 + glm::vec3(0.0f, 0.0f, 4.0f),
        .ref        = std::ref(s_pos1),
        .durationMs = 2000,
        .easingFn   = okay::easing::linear,
        .numLoops   = -1,
        .inOutBack  = true,
    })->start();

    okay::Tween<glm::vec3>::create(okay::TweenConfig<glm::vec3>{
        .start      = s_pos2,
        .end        = s_pos2 + glm::vec3(3.0f, 5.0f, 0.0f),
        .ref        = std::ref(s_pos2),
        .durationMs = 2000,
        .easingFn   = okay::easing::linear,
        .numLoops   = -1,
        .inOutBack  = true,
        .prefixMs   = 400,
    })->start();

    okay::Tween<glm::vec3>::create(okay::TweenConfig<glm::vec3>{
        .start      = s_pos3,
        .end        = s_pos3 + glm::vec3(-3.0f, 5.0f, 3.0f),
        .ref        = std::ref(s_pos3),
        .durationMs = 2000,
        .easingFn   = okay::easing::linear,
        .numLoops   = -1,
        .inOutBack  = true,
        .prefixMs   = 800,
    })->start();

    okay::Tween<glm::vec3>::create(okay::TweenConfig<glm::vec3>{
        .start      = s_pos4,
        .end        = s_pos4 + glm::vec3(4.0f, 10.0f, 0.0f),
        .ref        = std::ref(s_pos4),
        .durationMs = 2000,
        .easingFn   = okay::easing::linear,
        .numLoops   = -1,
        .inOutBack  = true,
        .prefixMs   = 200,
    })->start();
}

static void __gameUpdate() {
    // Camera orbits around origin
    // float theta = okay::Engine.time->timeSinceStartSec() * 0.05f * glm::pi<float>() * 2.0f;
    float theta = 0.0f;
    float dist = 15.0f;
    glm::vec3 camPos = glm::vec3(sin(theta) * dist, 2.0f, cos(theta) * dist);

    auto& cameraTransform = s_camera.getComponent<okay::TransformComponent>().value();
    cameraTransform->position = camPos;
    cameraTransform.lookAt(s_camera, glm::vec3{});

    s_teapot1.getComponent<okay::TransformComponent>().value()->position = s_pos1;
    s_teapot2.getComponent<okay::TransformComponent>().value()->position = s_pos2;
    s_teapot3.getComponent<okay::TransformComponent>().value()->position = s_pos3;
    s_teapot4.getComponent<okay::TransformComponent>().value()->position = s_pos4;
}

static void __gameShutdown() {
    okay::Engine.logger.info("Game shutdown.");
}
