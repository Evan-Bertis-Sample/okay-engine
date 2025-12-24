#include "okay_system.hpp"

using namespace okay;

std::array<OkaySystemPool, OkaySystemScope::SCOPE_COUNT> OkaySystemManager::_pools;

okay::IOkaySystem* okay::OkaySystemPool::nthPtr(std::size_t i) {
    auto it = _systems.begin();
    std::advance(it, static_cast<std::ptrdiff_t>(i));
    return it->second.get();
}
