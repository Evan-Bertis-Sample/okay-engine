#ifndef __IMGUI_IMPL_H__
#define __IMGUI_IMPL_H__

#include <imgui.h>
#include <memory>

namespace okay {

class IMGUIImpl {
   public:
    IMGUIImpl();
    ~IMGUIImpl();

    bool imguiSupported();
    void init(void* window, ImGuiConfigFlags flags, bool enableCallbacks);
    void newFrame();
    void renderDrawData(ImDrawData* drawData);
    void shutdown();

    ImGuiContext* getImguiContext() const;

   private:
    struct Context;
    std::unique_ptr<Context> _context;
};

};  // namespace okay

#endif  // __IMGUI_IMPL_H__
