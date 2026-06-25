#include <okay/core/engine/system.hpp>
#include <okay/runtime/runtime.hpp>

int main(int argc, char* args[]) {
    okay::Runtime.logger.debug("Editor Runtime!");
    auto game = create(argc, args);
    game.run();
}
