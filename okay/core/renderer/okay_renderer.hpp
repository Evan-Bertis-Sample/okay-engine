#ifndef __OKAY_RENDERER_H__
#define __OKAY_RENDERER_H__

#include <iostream>
#include <okay/core/renderer/okay_model.hpp>
#include <okay/core/renderer/okay_primitive.hpp>
#include <okay/core/renderer/okay_surface.hpp>
#include <okay/core/system/okay_system.hpp>

namespace okay {

struct OkayRendererSettings {
    SurfaceConfig SurfaceConfig;
};

// temporary just to get something displaying on the screen
static const char *VertexSrcSimple = "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "void main()\n"
    "{\n"
    "   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
    "}\0";

static const char *FragSrcSimple = "#version 330 core\n"
    "out vec4 FragColor;\n"
    "void main()\n"
    "{\n"
    "   FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);\n"
    "}\n\0";

class OkayRenderer : public OkaySystem<OkaySystemScope::ENGINE> {
   public:
    static std::unique_ptr<OkayRenderer> create(const OkayRendererSettings& settings) {
        return std::make_unique<OkayRenderer>(settings);
    }

    OkayRenderer(const OkayRendererSettings& settings)
        : _settings(settings), _surface(std::make_unique<Surface>(settings.SurfaceConfig)) {}

    void initialize() override {
        std::cout << "Okay Renderer initialized." << std::endl;
        _surface->initialize();

        OkayModel model = _modelBuffer.AddModel(
            okay::primitives::box()
                .withCenter({10, 10, 10})
                .withSize({10, 10, 10})
                .build()
        );
    }

    void postInitialize() override {
        std::cout << "Okay Renderer post-initialization." << std::endl;
        // Additional setup after initialization
    }

    void tick() override {
        _surface->pollEvents();
        // Render the current frame
    }

    void postTick() override {
        // Cleanup or prepare for the next frame
    }

    void shutdown() override {
        std::cout << "Okay Renderer shutdown." << std::endl;
        // Cleanup rendering resources here
    }

   private:
    OkayRendererSettings _settings;
    std::unique_ptr<Surface> _surface;

    OkayModelBuffer _modelBuffer;
};

}  // namespace okay

#endif  // __OKAY_RENDERER_H__