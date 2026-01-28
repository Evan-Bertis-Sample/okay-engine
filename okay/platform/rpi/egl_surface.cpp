#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <gbm.h>
#include <unistd.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <fcntl.h>
#include <atomic>
#include <csignal>
#include <okay/core/renderer/okay_surface.hpp>
#include <stdexcept>

#ifndef DRM_DEVICE_PATH
#define DRM_DEVICE_PATH "/dev/dri/card0"
#endif

namespace {

std::atomic<bool> g_quit{false};
void sigint_handler(int) {
    g_quit.store(true);
}

uint32_t bo_to_fb(gbm_bo* bo, int drmFd) {
    uint32_t handles[4]{}, strides[4]{}, offsets[4]{};
    uint64_t modifiers[4]{};
    const uint32_t fmt = gbm_bo_get_format(bo);
    const int planes = gbm_bo_get_plane_count(bo);
    for (int i = 0; i < planes; ++i) {
        handles[i] = gbm_bo_get_handle_for_plane(bo, i).u32;
        strides[i] = gbm_bo_get_stride_for_plane(bo, i);
        offsets[i] = gbm_bo_get_offset(bo, i);
        modifiers[i] = gbm_bo_get_modifier(bo);
    }
    uint32_t fb = 0;
    int ret;
    if (modifiers[0]) {
        ret = drmModeAddFB2WithModifiers(drmFd, gbm_bo_get_width(bo), gbm_bo_get_height(bo), fmt,
                                         handles, strides, offsets, modifiers, &fb,
                                         DRM_MODE_FB_MODIFIERS);
    } else {
        ret = drmModeAddFB2(drmFd, gbm_bo_get_width(bo), gbm_bo_get_height(bo), fmt, handles,
                            strides, offsets, &fb, 0);
    }
    return (ret == 0) ? fb : 0;
}

}  // namespace

namespace okay {

struct Surface::SurfaceImpl {
    SurfaceConfig cfg;

    // DRM/KMS
    int drmFd = -1;
    drmModeRes* res = nullptr;
    drmModeConnector* conn = nullptr;
    drmModeEncoder* enc = nullptr;
    drmModeCrtc* origCrtc = nullptr;
    uint32_t crtcId = 0, connectorId = 0;
    drmModeModeInfo mode{};

    // GBM/EGL
    gbm_device* gbmDev = nullptr;
    gbm_surface* gbmSurf = nullptr;

    EGLDisplay dpy = EGL_NO_DISPLAY;
    EGLConfig cfgEGL = nullptr;
    EGLContext ctx = EGL_NO_CONTEXT;
    EGLSurface surf = EGL_NO_SURFACE;

    // front buffer tracking
    gbm_bo* frontBo = nullptr;
    uint32_t frontFb = 0;

    explicit SurfaceImpl(const SurfaceConfig& c) : cfg(c) {}
};

Surface::Surface(const SurfaceConfig& cfg) : _impl(std::make_unique<SurfaceImpl>(cfg)) {}
Surface::~Surface() {
    destroy();
}
Surface::Surface(Surface&&) noexcept = default;
Surface& Surface::operator=(Surface&&) noexcept = default;

void Surface::initialize() {
    // ctrl+c to exit loop
    std::signal(SIGINT, sigint_handler);

    // open DRM
    _impl->drmFd = open(DRM_DEVICE_PATH, O_RDWR | O_CLOEXEC);
    if (_impl->drmFd < 0) throw std::runtime_error("open DRM device failed");

    _impl->res = drmModeGetResources(_impl->drmFd);
    if (!_impl->res) throw std::runtime_error("drmModeGetResources failed");

    // choose a connected connector
    for (int i = 0; i < _impl->res->count_connectors; ++i) {
        drmModeConnector* c = drmModeGetConnector(_impl->drmFd, _impl->res->connectors[i]);
        if (c && c->connection == DRM_MODE_CONNECTED && c->count_modes > 0) {
            _impl->conn = c;
            break;
        }
        drmModeFreeConnector(c);
    }
    if (!_impl->conn) throw std::runtime_error("no connected DRM connector");
    _impl->connectorId = _impl->conn->connector_id;

    // mode: prefer preferred
    bool preferred = false;
    for (int i = 0; i < _impl->conn->count_modes; ++i) {
        auto& m = _impl->conn->modes[i];
        if (m.type & DRM_MODE_TYPE_PREFERRED) {
            _impl->mode = m;
            preferred = true;
            break;
        }
    }
    if (!preferred) _impl->mode = _impl->conn->modes[0];

    // encoder + crtc
    _impl->enc = _impl->conn->encoder_id ? drmModeGetEncoder(_impl->drmFd, _impl->conn->encoder_id)
                                         : nullptr;
    if (!_impl->enc) {
        for (int i = 0; i < _impl->res->count_encoders && !_impl->enc; ++i)
            _impl->enc = drmModeGetEncoder(_impl->drmFd, _impl->res->encoders[i]);
    }
    if (!_impl->enc) throw std::runtime_error("no DRM encoder");
    _impl->crtcId = _impl->enc->crtc_id ? _impl->enc->crtc_id : _impl->res->crtcs[0];
    _impl->origCrtc = drmModeGetCrtc(_impl->drmFd, _impl->crtcId);

    // gbm
    _impl->gbmDev = gbm_create_device(_impl->drmFd);
    if (!_impl->gbmDev) throw std::runtime_error("gbm_create_device failed");

    const uint32_t w = _impl->mode.hdisplay;
    const uint32_t h = _impl->mode.vdisplay;
    _impl->gbmSurf = gbm_surface_create(_impl->gbmDev, w, h, GBM_FORMAT_ARGB8888,
                                        GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);
    if (!_impl->gbmSurf) throw std::runtime_error("gbm_surface_create failed");

    // egl (desktop GL via EGL)
    auto getPlatformDisplay = reinterpret_cast<PFNEGLGETPLATFORMDISPLAYEXTPROC>(
        eglGetProcAddress("eglGetPlatformDisplayEXT"));
    _impl->dpy = getPlatformDisplay
                     ? getPlatformDisplay(EGL_PLATFORM_GBM_KHR, _impl->gbmDev, nullptr)
                     : eglGetDisplay((EGLNativeDisplayType)_impl->gbmDev);
    if (_impl->dpy == EGL_NO_DISPLAY) throw std::runtime_error("eglGetDisplay failed");

    if (!eglInitialize(_impl->dpy, nullptr, nullptr))
        throw std::runtime_error("eglInitialize failed");
    if (!eglBindAPI(EGL_OPENGL_API)) throw std::runtime_error("eglBindAPI(EGL_OPENGL_API) failed");

    const EGLint cfgAttribs[] = {EGL_SURFACE_TYPE,
                                 EGL_WINDOW_BIT,
                                 EGL_RENDERABLE_TYPE,
                                 EGL_OPENGL_BIT,
                                 EGL_RED_SIZE,
                                 8,
                                 EGL_GREEN_SIZE,
                                 8,
                                 EGL_BLUE_SIZE,
                                 8,
                                 EGL_ALPHA_SIZE,
                                 8,
                                 EGL_NONE};
    EGLint n = 0;
    if (!eglChooseConfig(_impl->dpy, cfgAttribs, &_impl->cfgEGL, 1, &n) || n == 0)
        throw std::runtime_error("eglChooseConfig failed");

    _impl->surf = eglCreateWindowSurface(_impl->dpy, _impl->cfgEGL,
                                         (EGLNativeWindowType)_impl->gbmSurf, nullptr);
    if (_impl->surf == EGL_NO_SURFACE) throw std::runtime_error("eglCreateWindowSurface failed");

    _impl->ctx = eglCreateContext(_impl->dpy, _impl->cfgEGL, EGL_NO_CONTEXT, nullptr);
    if (_impl->ctx == EGL_NO_CONTEXT) throw std::runtime_error("eglCreateContext failed");

    if (!eglMakeCurrent(_impl->dpy, _impl->surf, _impl->surf, _impl->ctx))
        throw std::runtime_error("eglMakeCurrent failed");

    eglSwapInterval(_impl->dpy, _impl->cfg.vsync ? 1 : 0);
}

bool Surface::shouldClose() const {
    return g_quit.load();
}

void Surface::pollEvents() {
    // no windowing system here; add evdev if you want input
}

void Surface::swapBuffers() {
    eglSwapBuffers(_impl->dpy, _impl->surf);

    gbm_bo* next = gbm_surface_lock_front_buffer(_impl->gbmSurf);
    if (!next) throw std::runtime_error("gbm_surface_lock_front_buffer failed");
    uint32_t fb = bo_to_fb(next, _impl->drmFd);
    if (!fb) {
        gbm_surface_release_buffer(_impl->gbmSurf, next);
        throw std::runtime_error("drmModeAddFB2 failed");
    }

    if (drmModeSetCrtc(_impl->drmFd, _impl->crtcId, fb, 0, 0, &_impl->connectorId, 1,
                       &_impl->mode) != 0) {
        drmModeRmFB(_impl->drmFd, fb);
        gbm_surface_release_buffer(_impl->gbmSurf, next);
        throw std::runtime_error("drmModeSetCrtc failed");
    }

    if (_impl->frontFb) {
        drmModeRmFB(_impl->drmFd, _impl->frontFb);
        _impl->frontFb = 0;
    }
    if (_impl->frontBo) {
        gbm_surface_release_buffer(_impl->gbmSurf, _impl->frontBo);
        _impl->frontBo = nullptr;
    }
    _impl->frontBo = next;
    _impl->frontFb = fb;
}

void Surface::destroy() {
    if (_impl->origCrtc && _impl->drmFd >= 0 && _impl->crtcId) {
        drmModeSetCrtc(_impl->drmFd, _impl->origCrtc->crtc_id, _impl->origCrtc->buffer_id,
                       _impl->origCrtc->x, _impl->origCrtc->y, &_impl->connectorId, 1,
                       &_impl->origCrtc->mode);
    }

    if (_impl->frontFb) {
        drmModeRmFB(_impl->drmFd, _impl->frontFb);
        _impl->frontFb = 0;
    }
    if (_impl->frontBo) {
        gbm_surface_release_buffer(_impl->gbmSurf, _impl->frontBo);
        _impl->frontBo = nullptr;
    }

    if (_impl->dpy != EGL_NO_DISPLAY) {
        eglMakeCurrent(_impl->dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (_impl->surf != EGL_NO_SURFACE) eglDestroySurface(_impl->dpy, _impl->surf);
        if (_impl->ctx != EGL_NO_CONTEXT) eglDestroyContext(_impl->dpy, _impl->ctx);
        eglTerminate(_impl->dpy);
        _impl->dpy = EGL_NO_DISPLAY;
    }

    if (_impl->gbmSurf) {
        gbm_surface_destroy(_impl->gbmSurf);
        _impl->gbmSurf = nullptr;
    }
    if (_impl->gbmDev) {
        gbm_device_destroy(_impl->gbmDev);
        _impl->gbmDev = nullptr;
    }

    if (_impl->enc) {
        drmModeFreeEncoder(_impl->enc);
        _impl->enc = nullptr;
    }
    if (_impl->conn) {
        drmModeFreeConnector(_impl->conn);
        _impl->conn = nullptr;
    }
    if (_impl->res) {
        drmModeFreeResources(_impl->res);
        _impl->res = nullptr;
    }
    if (_impl->origCrtc) {
        drmModeFreeCrtc(_impl->origCrtc);
        _impl->origCrtc = nullptr;
    }
    if (_impl->drmFd >= 0) {
        close(_impl->drmFd);
        _impl->drmFd = -1;
    }
}

}  // namespace okay
