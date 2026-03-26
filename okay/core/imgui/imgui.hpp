#ifndef _IMGUI_H__
#define _IMGUI_H__

#include <okay/core/engine/system.hpp>

namespace okay {

class IMGUISystem : public System<SystemScope::GAME> {
   public:
    IMGUISystem() = default;

    void initialize();

    void postInitialize();

    void tick();

    void shutdown();

   private:
    bool _initialized{false};
};

}  // namespace okay

#endif
