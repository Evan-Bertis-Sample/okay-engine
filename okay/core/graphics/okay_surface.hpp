#ifndef __OKAY_SURFACE_H__
#define __OKAY_SURFACE_H__

namespace okay {

/// @brief Configuration for the surface.
struct SurfaceConfig {
    int width = 800;
    int height = 600;
    const char* title = "Okay Surface";
    bool resizable = true;
    bool vsync = true;
};

/// @brief Abstract interface for the implementation of a surface.
class SurfaceDriver {
   public:
    SurfaceDriver(const SurfaceConfig& config) : _config(config) {}
    virtual ~SurfaceDriver() = default;

    virtual void initialize() = 0;
    virtual bool shouldClose() const = 0;
    virtual void pollEvents() = 0;
    virtual void swapBuffers() = 0;
    virtual void destroy() = 0;

   protected:
    SurfaceConfig _config;
};


};  // namespace okay

#endif  // __OKAY_SURFACE_H__