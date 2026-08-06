// editor_main.cpp

#include <okay/core/engine/engine.hpp>
#include <okay/core/engine/resource.hpp>
#include <okay/core/engine/system.hpp>
#include <okay/runtime/editor/proc_interface.hpp>
#include <okay/runtime/runtime.hpp>

#include <csignal>
#include <dynalo/dynalo.hpp>

using namespace okay;

static std::uint16_t s_hotReloadCount{0};

static void __exitSignal(int sig) {
    okay::Engine.logger.info("Exit signal received: {}", sig);
    okay::Engine.shutdown();
}

std::string getLibDynamicName() {
    return dynalo::to_native_name(
        std::format("./game_dll/lib" OKAY_GAME_NAME "_{}", s_hotReloadCount));
};

int main(int argc, char* args[]) {
    Runtime.logger.debug("Editor Runtime!");

    // attach an interrupt to exit the program on ctrl c
    std::signal(SIGINT, __exitSignal);

    dynalo::library lib(getLibDynamicName());
    auto createGameFn = lib.get_function<void(Game*, int, char*[])>("create");

    if (!createGameFn) {
        Runtime.logger.error("Unable to find createGameFn!");
    }

    Game game;
    createGameFn(&game, argc, args);

    editor::ProcInterface interface;
    interface.initialize();

    interface.addCallback(editor::ProcContentKind::HOT_RELOAD_ASSETS,
        [&](editor::ProcMessageHeader header, std::span<uint8_t> data) {
            Runtime.logger.debug("Reloading assets!");
        });

    interface.addCallback(editor::ProcContentKind::HOT_RELOAD_CODE,
        [&](editor::ProcMessageHeader header, std::span<uint8_t> data) {
            Runtime.logger.debug("Reloading code!");
            if (header.payloadLength != 2) {
                Runtime.logger.error(
                    "Unexpected payload in hot reload message! Expected a length of 1, got {}",
                    header.payloadLength);
                return;
            }

            s_hotReloadCount = *reinterpret_cast<std::uint16_t*>(&data[0]);
            Engine.logger.debug("Reloading {}", getLibDynamicName());

            ReloadContext context;
            game.prepareForReload(context);

            lib = dynalo::library(getLibDynamicName());
            auto createGameFn = lib.get_function<void(Game*, int, char*[])>("create");
            if (!createGameFn) {
                Runtime.logger.error("Unable to find createGameFn!");
                return;
            }

            Engine.resources.clearResources(ResourceScope::RUNTIME);
            Engine.resources.clearResources(ResourceScope::ENGINE);
            Engine.resources.clearResources(ResourceScope::GAME);
            Engine.resources.clearResources(ResourceScope::LEVEL);

            createGameFn(&game, argc, args);
            game.initialize();
            game.reload(context);
        });

    game.initialize();
    while (Engine.shouldRun()) {
        interface.tick();
        game.tick();
    }
    game.shutdown();

    return 0;
}
