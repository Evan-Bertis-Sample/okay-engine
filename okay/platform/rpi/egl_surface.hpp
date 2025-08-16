#ifndef __EGL_SURFACE_H__
#define __EGL_SURFACE_H__

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <gbm.h>
#include <signal.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

#include <atomic>
#include <cstdint>
#include <memory>
#include <okay/core/renderer/okay_surface.hpp> 
#include <okay/core/system/okay_system.hpp>
#include <okay/core/util/option.hpp>

namespace okay::egl {

/// @brief EGL + GBM (KMS) Surface driver for headless Raspberry Pi OS Lite.
/// Fullscreen only (no X/Wayland). Uses /dev/dri/card0.
class EGLSurface : public SurfaceDriver {
   public:
    explicit EGLSurface(const SurfaceConfig& cfg);
    ~EGLSurface() override;

    void initialize() override;
    bool shouldClose() const override;
    void pollEvents() override;   // no-op (KMS)
    void swapBuffers() override;  // page-flip via DRM
    void destroy() override;

   private:
    // DRM / KMS
    int _drmFd = -1;
    drmModeRes* _drmRes = nullptr;
    drmModeConnector* _connector = nullptr;
    drmModeEncoder* _encoder = nullptr;
    drmModeCrtc* _origCrtc = nullptr;
    uint32_t _crtcId = 0;
    uint32_t _connectorId = 0;
    drmModeModeInfo _mode{};  // chosen display mode (prefer "preferred")

    // GBM
    gbm_device* _gbmDev = nullptr;
    gbm_surface* _gbmSurf = nullptr;

    // EGL
    EGLDisplay _eglDisplay = EGL_NO_DISPLAY;
    EGLConfig _eglConfig = nullptr;
    EGLContext _eglContext = EGL_NO_CONTEXT;
    EGLSurface _eglSurface = EGL_NO_SURFACE;

    // Page flip bookkeeping
    gbm_bo* _frontBo = nullptr;
    uint32_t _frontFb = 0;

    // GL version chosen
    int _glesMajor = 0;

    // graceful shutdown via Ctrl+C
    static std::atomic<bool>& _quitFlag();

    // helpers
    void _openDrm();
    void _pickConnectorAndMode();
    void _createGbm();
    void _createEgl();
    void _makeCurrent();
    void _restoreCrtc();
    static void _sigintHandler(int);

    static uint32_t _boToFb(gbm_bo* bo, int drmFd);
    static void _rmFbIfAny(int drmFd, uint32_t& fb);
    static void _releaseBoIfAny(gbm_surface* surf, gbm_bo*& bo);
};

}  // namespace okay::egl

#endif  // __EGL_SURFACE_H__
