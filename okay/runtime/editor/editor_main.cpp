// editor_main.cpp

#include <okay/core/engine/system.hpp>
#include <okay/runtime/editor/proc_interface.hpp>
#include <okay/runtime/runtime.hpp>

#include <dynalo/dynalo.hpp>

int main(int argc, char* args[]) {
    okay::Runtime.logger.debug("Editor Runtime!");
    dynalo::library lib(dynalo::to_native_name("lib" OKAY_GAME_NAME));

    auto createGameFn = lib.get_function<void(okay::Game*, int, char*[])>("create");

    if (!createGameFn) {
        okay::Runtime.logger.error("Unable to find createGameFn!");
    }
    okay::Game game;
    createGameFn(&game, argc, args);
    okay::editor::ProcInterface interface;
    interface.initialize();

    while (true) {
        interface.tick();
    }

    game.run();
}
