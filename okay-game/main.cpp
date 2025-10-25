#include <okay/core/okay.hpp>
#include <okay/core/renderer/okay_renderer.hpp>
#include <okay/core/renderer/okay_surface.hpp>


static void __gameInitialize();
static void __gameUpdate();
static void __gameShutdown();

int main() {
    okay::SurfaceConfig surfaceConfig;
    okay::Surface surface(surfaceConfig);

    okay::OkayRendererSettings rendererSettings{surfaceConfig};
    auto renderer = okay::OkayRenderer::create(rendererSettings);

    okay::OkayGame::create()
        .addSystems(
            std::move(renderer)
        )
        .onInitialize(__gameInitialize)
        .onUpdate(__gameUpdate)
        .onShutdown(__gameShutdown)
        .run();

    return 0;
}

static void __gameInitialize() {
    std::cout << "Game initialized." << std::endl;
    // Additional game initialization logic
}

static void __gameUpdate() {
    std::cout << "Game updated." << std::endl;
    // Game update logic
}

static void __gameShutdown() {
    std::cout << "Game shutdown." << std::endl;
    // Cleanup logic before game shutdown
}