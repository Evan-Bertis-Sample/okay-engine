#include "okay_system.hpp"

using namespace okay;

std::array<std::unique_ptr<OkaySystemPool>, OkaySystemScope::__COUNT> OkaySystemManager::_pools;