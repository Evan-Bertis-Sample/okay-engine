// editor_main.cpp

#include <okay/core/engine/engine.hpp>
#include <okay/core/engine/system.hpp>
#include <okay/runtime/editor/proc_interface.hpp>
#include <okay/runtime/runtime.hpp>

#include <dynalo/dynalo.hpp>

using namespace okay;

int main(int argc, char* args[]) {
    Runtime.logger.debug("Editor Runtime!");
    dynalo::library lib(dynalo::to_native_name("lib" OKAY_GAME_NAME));

    auto createGameFn = lib.get_function<void(Game*, int, char*[])>("create");

    if (!createGameFn) {
        Runtime.logger.error("Unable to find createGameFn!");
    }

    Game game;
    createGameFn(&game, argc, args);

    editor::ProcInterface interface;
    interface.initialize();

    interface.addCallback(editor::ProcContentKind::HOT_RELOAD_ASSETS,
        [](editor::ProcMessageHeader header, std::span<uint8_t> data) {
            Runtime.logger.debug("Reloading assets!");
        });

    interface.addCallback(editor::ProcContentKind::HOT_RELOAD_CODE,
        [](editor::ProcMessageHeader header, std::span<uint8_t> data) {
            Runtime.logger.debug("Reloading code!");
        });

    game.initialize();
    while (Engine.shouldRun()) {
        interface.tick();
        game.tick();
    }
    game.shutdown();
}
