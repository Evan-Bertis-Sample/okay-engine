#include <okay/core/okay.hpp>

struct GameConfig {
    std::int16_t width = 800;
};

static void __gameInitialize(GameConfig& config);
static void __gameUpdate(GameConfig& config);
static void __gameShutdown(GameConfig& config);

int main() {
    GameConfig config;

    okay::OkayGame<GameConfig>::create()
        .addSystems(

            )
        .onInitialize(__gameInitialize)
        .onUpdate(__gameUpdate)
        .onShutdown(__gameShutdown)
        .run(config);

    return 0;
}

static void __gameInitialize(GameConfig& config) {
    std::cout << "Game initialized." << std::endl;
    // Additional game initialization logic
}

static void __gameUpdate(GameConfig& config) {
    std::cout << "Game updated." << std::endl;
    // Game update logic
}

static void __gameShutdown(GameConfig& config) {
    std::cout << "Game shutdown." << std::endl;
    // Cleanup logic before game shutdown
}