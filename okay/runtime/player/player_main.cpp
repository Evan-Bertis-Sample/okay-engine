// player_main.cpp

#include <okay/core/engine/engine.hpp>
#include <okay/core/engine/system.hpp>
#include <okay/core/util/export.hpp>
#include <okay/runtime/runtime.hpp>

#include <csignal>

using namespace okay;

static void __exitSignal(int sig) {
    okay::Engine.logger.info("Exit signal received: {}", sig);
    okay::Engine.shutdown();
}

extern "C" OKAY_IMPORT void create(okay::Game* game, int argc, char* args[]);

int main(int argc, char* args[]) {
    Runtime.logger.debug("Editor Runtime!");

    // attach an interrupt to exit the program on ctrl c
    std::signal(SIGINT, __exitSignal);

    Game game;
    create(&game, argc, args);

    game.initialize();
    while (Engine.shouldRun()) {
        game.tick();
    }
    game.shutdown();

    return 0;
}
