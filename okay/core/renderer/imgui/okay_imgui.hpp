#ifndef __OKAY_IMGUI_H__
#define __OKAY_IMGUI_H__

#include <okay/core/system/okay_system.hpp>

namespace okay {

class OkayIMGUI : public OkaySystem<OkaySystemScope::GAME> {
   public:
    OkayIMGUI() = default;

    void initialize();

    void postInitialize();

    void tick();

    void shutdown();

   private:
    bool _initialized{false};
};

}  // namespace okay

#endif
