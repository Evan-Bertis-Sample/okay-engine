#include <okay/okay.hpp>
#include <utility>
#include <glm/glm.hpp>

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

    okay::OkayGame::create()
        .addSystems(std::move(renderer),
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
            view.entity.transform, renderComponent.material, renderComponent.mesh);
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
