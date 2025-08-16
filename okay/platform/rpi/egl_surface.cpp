#include "egl_surface.hpp"

#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>
#include <unistd.h>

#include <GLES3/gl3.h>   // OK even if we end up on ES2; you can gate includes if you prefer

#ifndef DRM_DEVICE_PATH
#define DRM_DEVICE_PATH "/dev/dri/card0"
#endif

namespace {

bool isConnectorConnected(const drmModeConnector* c) {
    return c && c->connection == DRM_MODE_CONNECTED && c->count_modes > 0;
}

drmModeConnector* getConnectorById(int fd, uint32_t id) {
    auto* c = drmModeGetConnector(fd, id);
    return c;
}

} // anon

namespace okay::egl {

// ---- static quit flag ----
std::atomic<bool>& EGLSurface::_quitFlag() {
    static std::atomic<bool> flag{false};
    return flag;
}

void EGLSurface::_sigintHandler(int) {
    _quitFlag().store(true);
}

// ---- ctor/dtor ----
EGLSurface::EGLSurface(const SurfaceConfig& cfg) : SurfaceDriver(cfg) {}
EGLSurface::~EGLSurface() {
    destroy();
}

// ---- lifecycle ----
void EGLSurface::initialize() {
    // Ctrl+C -> shouldClose()
    struct sigaction sa{};
    sa.sa_handler = &EGLSurface::_sigintHandler;
    sigaction(SIGINT, &sa, nullptr);

    _openDrm();
    _pickConnectorAndMode();
    _createGbm();
    _createEgl();
    _makeCurrent();

    // Set swap interval depending on vsync preference (0=off, 1=on)
    eglSwapInterval(_eglDisplay, _config.vsync ? 1 : 0);
}

bool EGLSurface::shouldClose() const {
    return _quitFlag().load();
}

void EGLSurface::pollEvents() {
    // No windowing system -> nothing to poll.
    // You could add keyboard input via evdev if you want.
}

void EGLSurface::swapBuffers() {
    if (_eglDisplay == EGL_NO_DISPLAY || _eglSurface == EGL_NO_SURFACE) return;

    // Renderers will have drawn to the backbuffer by now.
    eglSwapBuffers(_eglDisplay, _eglSurface);

    // Lock the new front buffer from GBM and present via KMS
    gbm_bo* nextBo = gbm_surface_lock_front_buffer(_gbmSurf);
    if (!nextBo) {
        throw std::runtime_error("gbm_surface_lock_front_buffer() failed");
    }

    uint32_t nextFb = _boToFb(nextBo, _drmFd);
    if (!nextFb) {
        gbm_surface_release_buffer(_gbmSurf, nextBo);
        throw std::runtime_error("Failed to create FB for GBM BO");
    }

    // On the first frame, set the CRTC; afterwards, just set again (simple path).
    // (You can upgrade to page-flip events for tear-free async flips later.)
    int ret = drmModeSetCrtc(
        _drmFd,
        _crtcId,
        nextFb,
        0, 0,
        &_connectorId, 1,
        &_mode
    );
    if (ret != 0) {
        _rmFbIfAny(_drmFd, nextFb);
        gbm_surface_release_buffer(_gbmSurf, nextBo);
        throw std::runtime_error("drmModeSetCrtc() failed");
    }

    // Release previous front buffer
    _rmFbIfAny(_drmFd, _frontFb);
    _releaseBoIfAny(_gbmSurf, _frontBo);

    _frontBo = nextBo;
    _frontFb = nextFb;
}

void EGLSurface::destroy() {
    // Restore original CRTC if we changed it
    _restoreCrtc();

    // Release GBM front buffer and FB
    _rmFbIfAny(_drmFd, _frontFb);
    _releaseBoIfAny(_gbmSurf, _frontBo);

    // Tear down EGL
    if (_eglDisplay != EGL_NO_DISPLAY) {
        eglMakeCurrent(_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (_eglSurface != EGL_NO_SURFACE) {
            eglDestroySurface(_eglDisplay, _eglSurface);
            _eglSurface = EGL_NO_SURFACE;
        }
        if (_eglContext != EGL_NO_CONTEXT) {
            eglDestroyContext(_eglDisplay, _eglContext);
            _eglContext = EGL_NO_CONTEXT;
        }
        eglTerminate(_eglDisplay);
        _eglDisplay = EGL_NO_DISPLAY;
    }

    // GBM
    if (_gbmSurf) {
        gbm_surface_destroy(_gbmSurf);
        _gbmSurf = nullptr;
    }
    if (_gbmDev) {
        gbm_device_destroy(_gbmDev);
        _gbmDev = nullptr;
    }

    // DRM
    if (_encoder) { drmModeFreeEncoder(_encoder); _encoder = nullptr; }
    if (_connector) { drmModeFreeConnector(_connector); _connector = nullptr; }
    if (_drmRes) { drmModeFreeResources(_drmRes); _drmRes = nullptr; }
    if (_origCrtc) { drmModeFreeCrtc(_origCrtc); _origCrtc = nullptr; }
    if (_drmFd >= 0) { close(_drmFd); _drmFd = -1; }
}

// ---- helpers ----
void EGLSurface::_openDrm() {
    _drmFd = open(DRM_DEVICE_PATH, O_RDWR | O_CLOEXEC);
    if (_drmFd < 0) {
        throw std::runtime_error(std::string("Failed to open ") + DRM_DEVICE_PATH);
    }

    _drmRes = drmModeGetResources(_drmFd);
    if (!_drmRes) {
        throw std::runtime_error("drmModeGetResources() failed");
    }
}

void EGLSurface::_pickConnectorAndMode() {
    // Pick the first connected connector with modes
    for (int i = 0; i < _drmRes->count_connectors; ++i) {
        drmModeConnector* c = drmModeGetConnector(_drmFd, _drmRes->connectors[i]);
        if (isConnectorConnected(c)) {
            _connector = c;
            break;
        }
        drmModeFreeConnector(c);
    }
    if (!_connector) {
        throw std::runtime_error("No connected DRM connector found");
    }
    _connectorId = _connector->connector_id;

    // Choose the preferred mode if available, else the first mode
    bool foundPreferred = false;
    for (int i = 0; i < _connector->count_modes; ++i) {
        const auto& m = _connector->modes[i];
        if (m.type & DRM_MODE_TYPE_PREFERRED) {
            _mode = m;
            foundPreferred = true;
            break;
        }
    }
    if (!foundPreferred) {
        _mode = _connector->modes[0];
    }

    // Find an encoder/CRTC
    if (_connector->encoder_id) {
        _encoder = drmModeGetEncoder(_drmFd, _connector->encoder_id);
    }
    if (!_encoder) {
        // Try the first encoder
        for (int i = 0; i < _drmRes->count_encoders && !_encoder; ++i) {
            _encoder = drmModeGetEncoder(_drmFd, _drmRes->encoders[i]);
        }
    }
    if (!_encoder) {
        throw std::runtime_error("Failed to get DRM encoder");
    }

    // Choose CRTC
    _crtcId = _encoder->crtc_id;
    if (!_crtcId) {
        // brute: pick any CRTC compatible with the encoder
        for (int i = 0; i < _drmRes->count_crtcs; ++i) {
            if (_encoder->possible_crtcs & (1 << i)) {
                _crtcId = _drmRes->crtcs[i];
                break;
            }
        }
    }
    if (!_crtcId) {
        throw std::runtime_error("Failed to pick DRM CRTC");
    }

    // Save original CRTC to restore on shutdown
    _origCrtc = drmModeGetCrtc(_drmFd, _crtcId);
}

void EGLSurface::_createGbm() {
    _gbmDev = gbm_create_device(_drmFd);
    if (!_gbmDev) {
        throw std::runtime_error("gbm_create_device() failed");
    }

    // Note: We use the KMS mode size (fullscreen). If you want to honor _config.width/height,
    // you’d need to choose a matching mode or do scaled rendering.
    const uint32_t w = static_cast<uint32_t>(_mode.hdisplay);
    const uint32_t h = static_cast<uint32_t>(_mode.vdisplay);

    _gbmSurf = gbm_surface_create(
        _gbmDev,
        w, h,
        GBM_FORMAT_ARGB8888,
        GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING
    );
    if (!_gbmSurf) {
        throw std::runtime_error("gbm_surface_create() failed");
    }
}

void EGLSurface::_createEgl() {
    // Prefer EGL_KHR_platform_gbm path
    auto eglGetPlatformDisplayEXT =
        reinterpret_cast<PFNEGLGETPLATFORMDISPLAYEXTPROC>(
            eglGetProcAddress("eglGetPlatformDisplayEXT"));

    if (!eglGetPlatformDisplayEXT) {
        // Fallback
        _eglDisplay = eglGetDisplay((EGLNativeDisplayType)_gbmDev);
    } else {
        _eglDisplay = eglGetPlatformDisplayEXT(EGL_PLATFORM_GBM_KHR, _gbmDev, nullptr);
    }

    if (_eglDisplay == EGL_NO_DISPLAY) {
        throw std::runtime_error("eglGetDisplay/eglGetPlatformDisplayEXT failed");
    }

    if (!eglInitialize(_eglDisplay, nullptr, nullptr)) {
        throw std::runtime_error("eglInitialize() failed");
    }

    if (!eglBindAPI(EGL_OPENGL_ES_API)) {
        throw std::runtime_error("eglBindAPI(EGL_OPENGL_ES_API) failed");
    }

    // Try GLES3 first
    const EGLint cfgAttribs[] = {
        EGL_SURFACE_TYPE,    EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
        EGL_RED_SIZE,        8,
        EGL_GREEN_SIZE,      8,
        EGL_BLUE_SIZE,       8,
        EGL_ALPHA_SIZE,      8,
        EGL_NONE
    };

    EGLint numCfg = 0;
    if (!eglChooseConfig(_eglDisplay, cfgAttribs, &_eglConfig, 1, &numCfg) || numCfg == 0) {
        // Fallback to GLES2
        const EGLint cfgAttribs2[] = {
            EGL_SURFACE_TYPE,    EGL_WINDOW_BIT,
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
            EGL_RED_SIZE,        8,
            EGL_GREEN_SIZE,      8,
            EGL_BLUE_SIZE,       8,
            EGL_ALPHA_SIZE,      8,
            EGL_NONE
        };
        if (!eglChooseConfig(_eglDisplay, cfgAttribs2, &_eglConfig, 1, &numCfg) || numCfg == 0) {
            throw std::runtime_error("eglChooseConfig() failed for ES3 and ES2");
        }
        _glesMajor = 2;
    } else {
        _glesMajor = 3;
    }

    // Create EGL window surface from GBM surface
    _eglSurface = eglCreateWindowSurface(_eglDisplay, _eglConfig,
                                         (EGLNativeWindowType)_gbmSurf, nullptr);
    if (_eglSurface == EGL_NO_SURFACE) {
        throw std::runtime_error("eglCreateWindowSurface() failed");
    }

    // Create context
    EGLint ctxAttribsES3[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };
    EGLint ctxAttribsES2[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
    const EGLint* ctxAttribs = (_glesMajor == 3) ? ctxAttribsES3 : ctxAttribsES2;

    _eglContext = eglCreateContext(_eglDisplay, _eglConfig, EGL_NO_CONTEXT, ctxAttribs);
    if (_eglContext == EGL_NO_CONTEXT && _glesMajor == 3) {
        // One more fallback (driver says ES3 config ok but context failed): try ES2
        _glesMajor = 2;
        _eglContext = eglCreateContext(_eglDisplay, _eglConfig, EGL_NO_CONTEXT, ctxAttribsES2);
    }
    if (_eglContext == EGL_NO_CONTEXT) {
        throw std::runtime_error("eglCreateContext() failed");
    }
}

void EGLSurface::_makeCurrent() {
    if (!eglMakeCurrent(_eglDisplay, _eglSurface, _eglSurface, _eglContext)) {
        throw std::runtime_error("eglMakeCurrent() failed");
    }
}

void EGLSurface::_restoreCrtc() {
    if (_origCrtc && _drmFd >= 0 && _crtcId) {
        drmModeSetCrtc(_drmFd,
                       _origCrtc->crtc_id,
                       _origCrtc->buffer_id,
                       _origCrtc->x, _origCrtc->y,
                       &_connectorId, 1,
                       &_origCrtc->mode);
    }
}

// Convert a GBM BO to a DRM framebuffer handle (FB ID)
uint32_t EGLSurface::_boToFb(gbm_bo* bo, int drmFd) {
    uint32_t handles[4]{};
    uint32_t strides[4]{};
    uint32_t offsets[4]{};
    uint64_t modifiers[4]{};

    const uint32_t fmt = gbm_bo_get_format(bo);
    const int      planes = gbm_bo_get_plane_count(bo);

    for (int i = 0; i < planes; ++i) {
        handles[i]   = gbm_bo_get_handle_for_plane(bo, i).u32;
        strides[i]   = gbm_bo_get_stride_for_plane(bo, i);
        offsets[i]   = gbm_bo_get_offset(bo, i);
        modifiers[i] = gbm_bo_get_modifier(bo);
    }

    uint32_t fb = 0;
    int ret;
    if (modifiers[0]) {
        ret = drmModeAddFB2WithModifiers(
            drmFd,
            gbm_bo_get_width(bo),
            gbm_bo_get_height(bo),
            fmt,
            handles, strides, offsets,
            modifiers,
            &fb,
            DRM_MODE_FB_MODIFIERS
        );
    } else {
        ret = drmModeAddFB2(
            drmFd,
            gbm_bo_get_width(bo),
            gbm_bo_get_height(bo),
            fmt,
            handles, strides, offsets,
            &fb, 0
        );
    }
    if (ret != 0) return 0;
    return fb;
}

void EGLSurface::_rmFbIfAny(int drmFd, uint32_t& fb) {
    if (fb) {
        drmModeRmFB(drmFd, fb);
        fb = 0;
    }
}

void EGLSurface::_releaseBoIfAny(gbm_surface* surf, gbm_bo*& bo) {
    if (bo) {
        gbm_surface_release_buffer(surf, bo);
        bo = nullptr;
    }
}

} // namespace okay::egl
