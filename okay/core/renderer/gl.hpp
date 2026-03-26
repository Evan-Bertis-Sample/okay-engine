#ifndef __OKAY_GL_H__
#define __OKAY_GL_H__

#include <glad/glad.h>
#include <okay/core/engine/engine.hpp>
#include <okay/core/util/result.hpp>

inline void glClearErrors() {
    while (glGetError() != GL_NO_ERROR) {
    }
}

inline const char* glErrName(GLenum e) {
    switch (e) {
        case GL_NO_ERROR:
            return "GL_NO_ERROR";
        case GL_INVALID_ENUM:
            return "GL_INVALID_ENUM";
        case GL_INVALID_VALUE:
            return "GL_INVALID_VALUE";
        case GL_INVALID_OPERATION:
            return "GL_INVALID_OPERATION";
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            return "GL_INVALID_FRAMEBUFFER_OPERATION";
        case GL_OUT_OF_MEMORY:
            return "GL_OUT_OF_MEMORY";
        default:
            return "UNKNOWN_GL_ERROR";
    }
}

#define GL_CHECK(x)                                                                                \
    do {                                                                                           \
        glClearErrors();                                                                           \
        (x);                                                                                       \
        GLenum _e = glGetError();                                                                  \
        if (_e != GL_NO_ERROR) {                                                                   \
            Engine.logger.error(#x " failed. OpenGL error: {} ({})", (unsigned)_e, glErrName(_e)); \
            Engine.shutdown();                                                                     \
        }                                                                                          \
    } while (0)

#define GL_CHECK_FAILABLE(x)                                                                 \
    do {                                                                                     \
        glClearErrors();                                                                     \
        (x);                                                                                 \
        GLenum _e = glGetError();                                                            \
        if (_e != GL_NO_ERROR) {                                                             \
            return okay::Failable::errorResult(                                              \
                std::string(#x) + " failed. OpenGL error: " + std::to_string((unsigned)_e) + \
                " (" + glErrName(_e) + ")");                                                 \
        }                                                                                    \
    } while (0)

#endif