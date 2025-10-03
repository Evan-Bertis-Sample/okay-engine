#ifndef __OKAY_RENDERER_H__
#define __OKAY_RENDERER_H__

#include <iostream>
#include <okay/core/renderer/okay_surface.hpp>
#include <okay/core/system/okay_system.hpp>

namespace okay {

struct OkayRendererSettings {
    SurfaceConfig SurfaceConfig;
};

class OkayRenderer : public OkaySystem<OkayRenderer> {
   public:
    static const OkaySystemScope scope = OkaySystemScope::ENGINE;

    static std::unique_ptr<OkayRenderer> create(const OkayRendererSettings& settings) {
        return std::make_unique<OkayRenderer>(settings);
    }

    OkayRenderer(const OkayRendererSettings& settings)
        : _settings(settings), _surface(std::make_unique<Surface>(settings.SurfaceConfig)) {}

    void initialize() override {
        std::cout << "Okay Renderer initialized." << std::endl;
        _surface->initialize();
    }

    void postInitialize() override {
        std::cout << "Okay Renderer post-initialization." << std::endl;
        // Additional setup after initialization
    }

    void tick() override {
        std::cout << "Rendering frame..." << std::endl;
        _surface->pollEvents();
        // Render the current frame
    }

    void postTick() override {
        std::cout << "Post-rendering tasks." << std::endl;
        // Cleanup or prepare for the next frame
    }

    void shutdown() override {
        std::cout << "Okay Renderer shutdown." << std::endl;
        // Cleanup rendering resources here
    }

   private:
    OkayRendererSettings _settings;
    std::unique_ptr<Surface> _surface;
};

}  // namespace okay

#endif  // __OKAY_RENDERER_H__