#include <okay/okay.hpp>

#include <glm/glm.hpp>
#include <utility>

static void __gameInitialize();
static void __gameUpdate();
static void __gameShutdown();

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
    okay::Renderer* renderer = okay::Engine.systems.getSystemChecked<okay::Renderer>();
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
