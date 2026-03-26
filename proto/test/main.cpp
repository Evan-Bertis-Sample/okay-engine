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
#include <okay/core/ecs/okay_ecs.hpp>
#include <okay/core/ecs/components/render_component.hpp>

#include <utility>
#include <okay/core/renderer/okay_material.hpp>
#include <okay/core/renderer/okay_texture.hpp>
#include <okay/core/renderer/okay_render_pipeline.hpp>
#include <okay/core/renderer/okay_uniform.hpp>
#include <okay/core/renderer/passes/scene_pass.hpp>
#include <glm/ext/quaternion_trigonometric.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/gtc/random.hpp>

#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/vector_float4.hpp>
#include <okay/core/renderer/materials/lit.hpp>
#include <okay/core/renderer/materials/unlit.hpp>
#include <okay/core/renderer/okay_font.hpp>
#include <okay/core/renderer/okay_mesh.hpp>
#include <okay/core/util/result.hpp>

static void __gameInitialize();
static void __gameUpdate();
static void __gameShutdown();

int main() {
    okay::SurfaceConfig surfaceConfig;
    surfaceConfig.width = 800;
    surfaceConfig.height = 480;
    okay::Surface surface(surfaceConfig);

    okay::OkayRendererSettings rendererSettings{
        .surfaceConfig = surfaceConfig,
        .pipeline = okay::OkayRenderPipeline::create(std::make_unique<okay::ScenePass>())};

    auto renderer = okay::OkayRenderer::create(std::move(rendererSettings));

    okay::OkayLevelManagerSettings levelManagerSettings;
    auto levelManager = okay::OkayLevelManager::create(levelManagerSettings);

    okay::OkayGame::create()
        .addSystems(std::move(renderer),
                    std::move(levelManager),
                    std::make_unique<okay::OkayAssetManager>(),
                    std::make_unique<okay::OkayECS>())
        .onInitialize(__gameInitialize)
        .onUpdate(__gameUpdate)
        .onShutdown(__gameShutdown)
        .run();

    return 0;
}

static void __gameInitialize() {
    // Additional game initialization logic
    okay::OkayRenderer* renderer = okay::Engine.systems.getSystemChecked<okay::OkayRenderer>();
    okay::OkayAssetManager* assetManager =
        okay::Engine.systems.getSystemChecked<okay::OkayAssetManager>();
    auto teapotRes = assetManager->loadEngineAssetSync<okay::OkayMeshData>("models/teapot.obj");
    if (teapotRes.isError()) {
        okay::Engine.logger.error("Failed to load mesh: {}", teapotRes.error());
        return;
    }

    okay::OkayTextureLoadSettings textureLoadSettings{.store =
                                                          okay::OkayTextureDataStore::mainStore()};
    okay::OkayTexture texture =
        assetManager
            ->loadEngineAssetSync<okay::OkayTexture>("textures/uv_test.jpg", textureLoadSettings)
            .value()
            .asset;
    okay::OkayMesh teapot = renderer->meshBuffer().addMesh(teapotRes.value().asset);

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

    okay::OkayShaderHandle shader = renderer->materialRegistry().registerShader(
        shaderLoadRes.value().asset.vertexShader, shaderLoadRes.value().asset.fragmentShader);
    auto materialProperties = std::make_unique<okay::LitMaterial>();
    materialProperties->color.set(glm::vec4(1.0f, 1.0f, 0.0f, 1.0f));
    materialProperties->albedo = texture;

    okay::OkayMaterialHandle material =
        renderer->materialRegistry().registerMaterial(shader, std::move(materialProperties));

    okay::OkayECS* ecs = okay::Engine.systems.getSystemChecked<okay::OkayECS>();
    ecs->registerComponentType<okay::RenderComponent>();
    ecs->createEntity().addComponent<okay::RenderComponent>(teapot, material);

    for (auto view : ecs->query<okay::RenderComponent>()) {
        okay::RenderComponent& renderComponent = std::get<okay::RenderComponent&>(view.components);
        
        renderer->world().addRenderEntity(
            view.entity.transform,
            renderComponent.material,
            renderComponent.mesh
        );
    }
}

static void __gameUpdate() {
    // move the camera in a circle, always looking at the origin
    okay::OkayRenderer* renderer = okay::Engine.systems.getSystemChecked<okay::OkayRenderer>();
    float theta = okay::Engine.time->timeSinceStartSec() * 0.05f * glm::pi<float>();
    glm::vec3 pos = glm::vec3(sin(theta) * 5.0f, 0.0f, cos(theta) * 5.0f);
    // rotation much look at origin
    renderer->world().camera().transform.position = pos;
    renderer->world().camera().lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
}

static void __gameShutdown() {
    // Cleanup logic before game shutdown
    okay::Engine.logger.info("Game shutdown.");
}
