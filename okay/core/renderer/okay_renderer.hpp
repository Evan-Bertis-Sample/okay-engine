#ifndef __OKAY_RENDERER_H__
#define __OKAY_RENDERER_H__

#include <okay/core/system/okay_system.hpp>
#include <okay/core/renderer/okay_surface.hpp>
#include <iostream>

namespace okay {

class OkayRenderer : public OkaySystem<OkayRenderer> {
   public:
    static constexpr OkaySystemScope scope = OkaySystemScope::ENGINE;

    void initialize() override {
        std::cout << "Okay Renderer initialized." << std::endl;
    }

    void postInitialize() override {
        std::cout << "Okay Renderer post-initialization." << std::endl;
        // Additional setup after initialization
    }

    void tick() override {
        std::cout << "Rendering frame..." << std::endl;
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

};


#endif // __OKAY_RENDERER_H__