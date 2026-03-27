#ifndef __SURFACE_H__
#define __SURFACE_H__

#include <okay/core/engine/system.hpp>
#include <okay/core/renderer/gl.hpp>

#include <memory>

namespace okay {

struct SurfaceConfig {
    int width = 800;
    int height = 600;
    const char* title = "Okay Surface";
    bool resizable = true;
    bool vsync = true;
};

class Surface {
   public:
    explicit Surface(const SurfaceConfig& cfg);
    ~Surface();  // non-virtual dtor is fine (no inheritance)
    Surface(Surface&&) noexcept;
    Surface& operator=(Surface&&) noexcept;

    void initialize();
    bool shouldClose() const;
    void pollEvents();
    void swapBuffers();
    void destroy();
    void* getWindow();

   private:
    struct SurfaceImpl;                  // defined in the backend .cpp
    std::unique_ptr<SurfaceImpl> _impl;  // opaque to callers
};

}  // namespace okay

#endif  // _SURFACE_H__
