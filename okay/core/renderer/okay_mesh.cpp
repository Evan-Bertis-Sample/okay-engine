#include "okay_mesh.hpp"

#include <okay/core/okay.hpp>
#include <okay/core/renderer/okay_gl.hpp>
#include <okay/core/logging/okay_logger.hpp>
#include "okay/core/util/result.hpp"

using namespace okay;

OkayMesh OkayMeshBuffer::addMesh(const OkayMeshData& mesh) {
    OkayMesh m{};

    // get the vertexOffset of the mesh
    m.vertexOffset = _bufferData.size() / OkayVertex::numFloats();
    m.vertexCount = mesh.vertices.size();

    m.indexOffset = _indices.size();
    m.indexCount = mesh.indices.size();

    // push the vertices of the mesh
    for (const OkayVertex& v : mesh.vertices) {
        // push this vertex into the buffer, compactly
        // position
        _bufferData.push_back(v.position.x);
        _bufferData.push_back(v.position.y);
        _bufferData.push_back(v.position.z);
        // normal
        _bufferData.push_back(v.normal.x);
        _bufferData.push_back(v.normal.y);
        _bufferData.push_back(v.normal.z);
        // color
        _bufferData.push_back(v.color.x);
        _bufferData.push_back(v.color.y);
        _bufferData.push_back(v.color.z);
        // uv
        _bufferData.push_back(v.uv.x);
        _bufferData.push_back(v.uv.y);
    }

    // Now add the indices, but with an offset
    for (std::uint32_t i : mesh.indices) {
        _indices.push_back(i + m.vertexOffset);
    }

    _dataOutofDate = true;

    return m;
}

void OkayMeshBuffer::removeMesh(const OkayMesh& mesh) {
    // update the indices that refer to vertices after this mesh -- their offset must be decremented

    // from mesh.indexOffset + mesh.indexCount ... end
    // decrement by mesh.vertexCount
    for (std::size_t i = mesh.indexOffset + mesh.indexCount; i < _indices.size(); ++i) {
        _indices[i] -= mesh.vertexCount;
    }

    // remove the indices
    _indices.erase(_indices.begin() + mesh.indexOffset,
                   _indices.begin() + mesh.indexOffset + mesh.indexCount);
    // remove the vertices
    _bufferData.erase(_bufferData.begin() + mesh.vertexOffset * OkayVertex::numFloats(),
                      _bufferData.begin() + mesh.vertexOffset * OkayVertex::numFloats() +
                          mesh.vertexCount * OkayVertex::numFloats());

    _dataOutofDate = true;
}

Failable OkayMeshBuffer::initVertexAttributes() {
    if (_hasInitVertexAttributes) return Failable::ok({});

    Engine.logger.debug("Initializing vertex attributes");

    // MUST have a current context here.
    GL_CHECK(glBindVertexArray(_vao));

    // This must be the VBO that contains OkayVertex data
    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, _vbo));

    GL_CHECK(glEnableVertexAttribArray(0));
    GL_CHECK(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(OkayVertex),
                                   reinterpret_cast<void*>(offsetof(OkayVertex, position))));

    GL_CHECK(glEnableVertexAttribArray(1));
    GL_CHECK(glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(OkayVertex),
                                   reinterpret_cast<void*>(offsetof(OkayVertex, normal))));

    GL_CHECK(glEnableVertexAttribArray(2));
    GL_CHECK(glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(OkayVertex),
                                   reinterpret_cast<void*>(offsetof(OkayVertex, color))));

    GL_CHECK(glEnableVertexAttribArray(3));
    GL_CHECK(glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(OkayVertex),
                                   reinterpret_cast<void*>(offsetof(OkayVertex, uv))));

    GL_CHECK(glBindVertexArray(0));
    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, 0));

    GLint vao=0, vbo=0;
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vao);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &vbo);
    Engine.logger.debug("attrib setup bindings: VAO={}, ARRAY_BUFFER={}", _vao, _vbo);

    _hasInitVertexAttributes = true;
    Engine.logger.debug("Vertex attributes initialized");
    return Failable::ok({});
}

Failable OkayMeshBuffer::bindMeshData() {
    if (!_dataOutofDate) {
        Engine.logger.debug("Mesh data already bound");
        return Failable::ok({});
    }

    Engine.logger.debug("Binding mesh data");

    if (_vao == 0) {
        glGenVertexArrays(1, &_vao);

        if (glGetError() != GL_NO_ERROR) {
            Engine.logger.error("Failed to create VAO : {}", glGetError());
            return Failable::errorResult("Failed to create VAO");
        }
    }
    
    if (_vbo == 0) {
        glGenBuffers(1, &_vbo);

        if (glGetError() != GL_NO_ERROR) {
            Engine.logger.error("Failed to create VBO : {}", glGetError());
            return Failable::errorResult("Failed to create VBO");
        }
    }

    if (_ebo == 0) {
        glGenBuffers(1, &_ebo);

        if (glGetError() != GL_NO_ERROR) {
            Engine.logger.error("Failed to create EBO : {}", glGetError());
            return Failable::errorResult("Failed to create EBO");
        }
    }

    GL_CHECK(glBindVertexArray(_vao));

    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, _vbo));
    GL_CHECK(glBufferData(
        GL_ARRAY_BUFFER, _bufferData.size() * sizeof(GLfloat), _bufferData.data(), GL_STATIC_DRAW));

    GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo));
    GL_CHECK(glBufferData(
        GL_ELEMENT_ARRAY_BUFFER, _indices.size() * sizeof(GLuint), _indices.data(), GL_STATIC_DRAW));

    auto r = initVertexAttributes();
    if (r.isError()) {
        GL_CHECK(glBindVertexArray(0));
        return r;
    }

    GL_CHECK(glBindVertexArray(0));

    Engine.logger.debug("Mesh data bound");
    _dataOutofDate = false;
    return Failable::ok({});
}

void OkayMeshBuffer::drawMesh(const OkayMesh& mesh) {
    if (_dataOutofDate) {
        Engine.logger.error("Mesh data not bound");
        return;
    }

    glBindVertexArray(_vao);

    void* start = reinterpret_cast<void*>(mesh.indexOffset * sizeof(GLuint));
    GLsizei count = static_cast<GLsizei>(mesh.indexCount);

    glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, start);

    glBindVertexArray(0);
}