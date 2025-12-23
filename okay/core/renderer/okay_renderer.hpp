#ifndef __OKAY_RENDERER_H__
#define __OKAY_RENDERER_H__

#include <iostream>
#include <okay/core/asset/generic/shader_loader.hpp>
#include <okay/core/asset/okay_asset.hpp>
#include <okay/core/renderer/okay_material.hpp>
#include <okay/core/renderer/okay_mesh.hpp>
#include <okay/core/renderer/okay_primitive.hpp>
#include <okay/core/renderer/okay_surface.hpp>
#include <okay/core/system/okay_system.hpp>
#include <okay/core/util/singleton.hpp>

namespace okay {

struct OkayRendererSettings {
    SurfaceConfig SurfaceConfig;
};

class OkayRenderer : public OkaySystem<OkaySystemScope::ENGINE> {
   public:
    static std::unique_ptr<OkayRenderer> create(const OkayRendererSettings& settings) {
        return std::make_unique<OkayRenderer>(settings);
    }

    OkayRenderer(const OkayRendererSettings& settings)
        : _settings(settings), _surface(std::make_unique<Surface>(settings.SurfaceConfig)) {}

    using TestMaterialUniforms =
        OkayMaterialUniformCollection<OkayMaterialUniform<glm::vec3, FixedString("u_color")>>;

    GLuint VBO, VAO;
    OkayShader<TestMaterialUniforms> shader;

    void initialize() override {
        std::cout << "Okay Renderer initialized." << std::endl;
        _surface->initialize();

        OkayMesh model =
            _modelBuffer.AddModel(okay::primitives::box().sizeSet({10, 10, 10}).build());

        Result<OkayAsset<OkayShader<TestMaterialUniforms>>> shaderRes =
            Engine.systems.getSystem<OkayAssetManager>()
                .value()
                ->loadEngineAssetSync<OkayShader<TestMaterialUniforms>>(
                    std::filesystem::path("shaders/test"));

        if (shaderRes.isError()) {
            std::cerr << "Failed to load shader: " << shaderRes.error() << std::endl;
            return;
        }

        shader = shaderRes.value().asset;
        Failable compileResult = shader.compile();

        Failable shaderSetup = catchResult(
            shader.compile()
                .then([&](NoneType nt) { return shader.set(); })
                .then([&](NoneType nt) { return shader.findUniformLocations(); })
                .then([&](NoneType nt) {
                    shader.uniforms.get<FixedString("u_color")>().set(glm::vec3(1.0f, 1.0f, 1.0f));
                    return shader.passDirtyUniforms();
                }),
            [](const std::string& failMessage) { Engine.logger.error("Cannot setup shader!"); });

        if (!shaderSetup) {
            return; // we have failed
        }

        Engine.logger.debug("Shader setup done! Test float {}", 0.0f);

        float vertices[] = {
            -0.5f, -0.5f, 0.0f,  // left
            0.5f,  -0.5f, 0.0f,  // right
            0.0f,  0.5f,  0.0f   // top
        };

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then
        // configure vertex attributes(s).
        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        // note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex
        // attribute's bound vertex buffer object so afterwards we can safely unbind
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO,
        // but this rarely happens. Modifying other VAOs requires a call to glBindVertexArray
        // anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
        glBindVertexArray(0);
    }

    void postInitialize() override {
        std::cout << "Okay Renderer post-initialization." << std::endl;
        // Additional setup after initialization
    }

    void tick() override {
        _surface->pollEvents();
        // Render the current frame

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // draw our first triangle
        Failable f = shader.set();
        if (f.isError()) {
            std::cerr << "Failed to set shader: " << f.error() << std::endl;
            while (true) {
            }
            return;
        }

        glBindVertexArray(VAO);  // seeing as we only have a single VAO there's no need to bind it
                                 // every time, but we'll do so to keep things a bit more organized
        glDrawArrays(GL_TRIANGLES, 0, 3);

        _surface->swapBuffers();
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

    OkayMeshBuffer _modelBuffer;
};

}  // namespace okay

#endif  // __OKAY_RENDERER_H__