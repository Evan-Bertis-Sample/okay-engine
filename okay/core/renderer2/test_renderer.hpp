// Copyright (c) 2026 Okay Engine contributors
// SPDX-License-Identifier: MIT
//
// test_renderer.hpp - Okay Engine

#pragma once

// Test renderer for the development of renderer v2
// directly accesses RHI for now

#include <okay/core/engine/system.hpp>
#include <okay/core/renderer/surface.hpp>
#include <okay/rhi/rhi.hpp>

namespace okay {

struct TestRendererSettings {
    SurfaceConfig surfaceConfig;
};

class TestRenderer : public System<SystemScope::ENGINE> {
   public:
    TestRenderer(TestRendererSettings settings)
        : _settings(settings),
          _surface(settings.surfaceConfig),
          _rhi(VkRhiConfig{.applicationName = settings.surfaceConfig.title}) {}

    void initialize() {
        _rhi.initialize();
    }

   private:
    TestRendererSettings _settings;
    Surface _surface;
    VkRhi _rhi;
};

}  // namespace okay
