#ifndef __RENDERER_UTIL_H__
#define __RENDERER_UTIL_H___

#include <okay/core/engine/engine.hpp>
#include <okay/core/renderer/material.hpp>
#include <okay/core/renderer/mesh.hpp>
#include <okay/core/renderer/renderer.hpp>
#include <okay/core/renderer/shader.hpp>

namespace okay {

inline Mesh mesh(const MeshData& data, SystemParameter<Renderer> renderer = nullptr) {
    return renderer->meshBuffer().addMesh(data);
}

inline ShaderHandle shaderHandle(Shader shader, SystemParameter<Renderer> renderer = nullptr) {
    return renderer->materialRegistry().registerShader(shader.vertexShader, shader.fragmentShader);
}

inline MaterialHandle materialHandle(ShaderHandle shaderHandle,
    std::unique_ptr<IMaterialPropertyCollection> uniforms,
    SystemParameter<Renderer> renderer = nullptr) {
    return renderer->materialRegistry().registerMaterial(shaderHandle, std::move(uniforms));
}

inline MaterialHandle materialHandle(Shader shader,
    std::unique_ptr<IMaterialPropertyCollection> uniforms,
    SystemParameter<Renderer> renderer = nullptr) {
    return renderer->materialRegistry().registerMaterial(shaderHandle(shader), std::move(uniforms));
}

};  // namespace okay

#endif  // __RENDERER_UTIL_H__