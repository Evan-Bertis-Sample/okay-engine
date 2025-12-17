#ifndef __OKAY_RENDERER_H__
#define __OKAY_RENDERER_H__

#include <iostream>
#include <okay/core/renderer/okay_mesh.hpp>
#include <okay/core/renderer/okay_primitive.hpp>
#include <okay/core/renderer/okay_surface.hpp>
#include <okay/core/system/okay_system.hpp>
#include <okay/core/asset/okay_asset.hpp>

namespace okay {

struct OkayRendererSettings {
    SurfaceConfig SurfaceConfig;
};

// temporary just to get something displaying on the screen
static const char* VertexSrcSimple =
    "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "void main()\n"
    "{\n"
    "   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
    "}\0";

static const char* FragSrcSimple =
    "#version 330 core\n"
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

    unsigned int shaderProgram;
    unsigned int VBO, VAO;

    void initialize() override {
        std::cout << "Okay Renderer initialized." << std::endl;
        _surface->initialize();

        OkayMesh model =
            _modelBuffer.AddModel(okay::primitives::box().sizeSet({10, 10, 10}).build());

        auto vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &VertexSrcSimple, NULL);
        glCompileShader(vertexShader);

        // check for shader compile errors
        int success;
        char infoLog[512];
        glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
            std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
        }
        // fragment shader
        unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &FragSrcSimple, NULL);
        glCompileShader(fragmentShader);
        // check for shader compile errors
        glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
            std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
        }
        // link shaders
        shaderProgram = glCreateProgram();
        glAttachShader(shaderProgram, vertexShader);
        glAttachShader(shaderProgram, fragmentShader);
        glLinkProgram(shaderProgram);
        // check for linking errors
        glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
            std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
        }
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

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
        glUseProgram(shaderProgram);
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