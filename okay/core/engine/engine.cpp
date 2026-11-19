#include "engine.hpp"

#include <okay/core/asset/asset.hpp>

namespace okay {

OkayEngine Engine;

const std::vector<OkaySystemDescriptor> Game::REQUIRED_SYSTEMS = {
    // OkaySystemDescriptor::create<AssetManager>(),
};

Game& Game::onInitialize(std::function<void()> callback) {
    _onInitialize = std::move(callback);
    return *this;
}
Game& Game::onUpdate(std::function<void()> callback) {
    _onUpdate = std::move(callback);
    return *this;
}
Game& Game::onShutdown(std::function<void()> callback) {
    _onShutdown = std::move(callback);
    return *this;
}
bool Game::initialize() {
    bool allRequiredSystems = true;

    for (OkaySystemDescriptor systemDescriptor : REQUIRED_SYSTEMS) {
        if (!Engine.systems.hasSystem(systemDescriptor.SysId)) {
            Engine.logger.error("Missing required system {}", systemDescriptor.SystemName);
            allRequiredSystems = false;
        }
    }

    if (!allRequiredSystems) {
        return false;
    }

    SystemPool& enginePool = Engine.systems.getPool(SystemScope::ENGINE);
    SystemPool& gamePool = Engine.systems.getPool(SystemScope::GAME);
    SystemPool& levelPool = Engine.systems.getPool(SystemScope::LEVEL);

    for (ISystem* system : enginePool) {
        system->initialize();
        if (!Engine.shouldRun())
            return false;
    }

    for (ISystem* system : gamePool) {
        system->initialize();
        if (!Engine.shouldRun())
            return false;
    }

    for (ISystem* system : levelPool) {
        system->initialize();
        if (!Engine.shouldRun())
            return false;
    }

    if (_onInitialize)
        _onInitialize();

    for (ISystem* system : enginePool) {
        system->postInitialize();
    }

    for (ISystem* system : gamePool) {
        system->postInitialize();
    }

    for (ISystem* system : levelPool) {
        system->postInitialize();
    }

    Engine.time->reset();
    return true;
}

void Game::prepareForReload(ReloadContext& context) {
    Engine.logger.debug("Preparing for reload...");

    SystemPool& enginePool = Engine.systems.getPool(SystemScope::ENGINE);
    SystemPool& gamePool = Engine.systems.getPool(SystemScope::GAME);
    SystemPool& levelPool = Engine.systems.getPool(SystemScope::LEVEL);

    for (ISystem* system : enginePool) {
        system->prepareForReload(context);
    }

    for (ISystem* system : gamePool) {
        system->prepareForReload(context);

        for (ISystem* system : levelPool) {
            system->prepareForReload(context);
        }
    }

    // Engine.systems.clear();
}

void Game::reload(ReloadContext& context) {
    Engine.logger.debug("Reloading systems...");

    SystemPool& enginePool = Engine.systems.getPool(SystemScope::ENGINE);
    SystemPool& gamePool = Engine.systems.getPool(SystemScope::GAME);
    SystemPool& levelPool = Engine.systems.getPool(SystemScope::LEVEL);

    for (ISystem* system : enginePool) {
        system->reload(context);
    }

    for (ISystem* system : gamePool) {
        system->reload(context);

        for (ISystem* system : levelPool) {
            system->reload(context);
        }
    }
}
void Game::tick() {
    SystemPool& enginePool = Engine.systems.getPool(SystemScope::ENGINE);
    SystemPool& gamePool = Engine.systems.getPool(SystemScope::GAME);
    SystemPool& levelPool = Engine.systems.getPool(SystemScope::LEVEL);

    for (ISystem* system : enginePool) {
        system->preTick();
    }

    for (ISystem* system : gamePool) {
        system->preTick();
    }

    for (ISystem* system : levelPool) {
        system->preTick();
    }

    for (ISystem* system : enginePool) {
        system->tick();
        if (!Engine.shouldRun())
            break;
    }

    for (ISystem* system : gamePool) {
        system->tick();
        if (!Engine.shouldRun())
            break;
    }

    for (ISystem* system : levelPool) {
        system->tick();
        if (!Engine.shouldRun())
            break;
    }

    if (_onUpdate)
        _onUpdate();

    for (ISystem* system : enginePool) {
        system->postTick();
    }

    for (ISystem* system : gamePool) {
        system->postTick();
    }

    for (ISystem* system : levelPool) {
        system->postTick();
    }

    Engine.time->updateDeltaTime();
    Engine._frameCount++;
}
void Game::shutdown() {
    SystemPool& enginePool = Engine.systems.getPool(SystemScope::ENGINE);
    SystemPool& gamePool = Engine.systems.getPool(SystemScope::GAME);
    SystemPool& levelPool = Engine.systems.getPool(SystemScope::LEVEL);

    for (ISystem* system : enginePool) {
        system->shutdown();
    }

    for (ISystem* system : gamePool) {
        system->shutdown();
    }

    for (ISystem* system : levelPool) {
        system->shutdown();
    }

    std::cout << "Shutdown location: " << Engine.shutdownLoc().file_name() << ":"
              << Engine.shutdownLoc().line() << std::endl;

    if (_onShutdown)
        _onShutdown();
}
void Game::run() {
    if (!initialize()) {
        return;
    }

    while (Engine.shouldRun()) {
        tick();
    }

    shutdown();
}

}  // namespace okay
