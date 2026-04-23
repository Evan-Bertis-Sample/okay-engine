#include "mesh.hpp"

#include "okay/core/util/result.hpp"

#include <okay/core/engine/engine.hpp>
#include <okay/core/engine/logger.hpp>
#include <okay/core/renderer/gl.hpp>

using namespace okay;

Mesh MeshBuffer::addMesh(const MeshData& mesh) {
    BlockMeta* blockPtr {};

    for (auto& block : _blocks) {
        if (block.isFree && block.vertexCount >= mesh.vertices.size() && block.indexCount >= mesh.indices.size()) {
            block.isFree = false;
            blockPtr = &block;
            break;
        }
    }

    Mesh m{};

    if (blockPtr != nullptr) {
        m.vertexOffset = blockPtr->vertexOffset;
        m.vertexCount = mesh.vertices.size();
        m.indexOffset = blockPtr->indexOffset;
        m.indexCount = mesh.indices.size();

        float* ptr { getMeshPointer(m) };

        for (const MeshVertex& v : mesh.vertices) {
            *ptr = v.position.x;
            ++ptr;
            *ptr = v.position.y;
            ++ptr;
            *ptr = v.position.z;
            ++ptr;
            *ptr = v.normal.x;
            ++ptr;
            *ptr = v.normal.y;
            ++ptr;
            *ptr = v.normal.z;
            ++ptr;
            *ptr = v.color.x;
            ++ptr;
            *ptr = v.color.y;
            ++ptr;
            *ptr = v.color.z;
            ++ptr;
            *ptr = v.uv.x;
            ++ptr;
            *ptr = v.uv.y;
        }

        for (std::size_t i {}; i < m.indexCount; ++i) {
            _indices[blockPtr->indexOffset + i] = mesh.indices[i] + m.vertexOffset;
        }

        _dataOutofDate = true;

        return m;
    }

    BlockMeta bm{};

    // get the vertexOffset of the mesh
    m.vertexOffset = _bufferData.size() / MeshVertex::numFloats();
    bm.vertexOffset = m.vertexOffset;
    m.vertexCount = mesh.vertices.size();
    bm.vertexCount = m.vertexCount;

    m.indexOffset = _indices.size();
    bm.indexOffset = m.indexOffset;
    m.indexCount = mesh.indices.size();
    bm.indexCount = m.indexCount;

    Bounds b = Bounds::none();

    // push the vertices of the mesh
    for (const MeshVertex& v : mesh.vertices) {
        // push this vertex into the buffer, compactly
        // position
        _bufferData.push_back(v.position.x);
        _bufferData.push_back(v.position.y);
        _bufferData.push_back(v.position.z);
        b.extend(v.position);
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

    _blocks.push_back(bm);

    return m;
}

void MeshBuffer::removeMesh(const Mesh& mesh) {
    // look for block to free
    for (auto& block : _blocks) {
        if (block.vertexOffset == mesh.vertexOffset) {
            block.isFree = true;
            break;
        }
    }

    _dataOutofDate = true;
}

float* MeshBuffer::getMeshPointer(const Mesh& mesh) {
    std::size_t floatIndex { mesh.vertexOffset * MeshVertex::numFloats() };

    if (floatIndex >= _bufferData.size()) {
        Engine.logger.error("getMeshPointer out of bounds");

        return nullptr;
    }

    return &_bufferData[floatIndex];
}

Failable MeshBuffer::initVertexAttributes() {
    if (_hasInitVertexAttributes)
        return Failable::ok({});

    Engine.logger.debug("Initializing vertex attributes");

    // create buffers
    GL_CHECK(glGenVertexArrays(1, &_vao));
    GL_CHECK(glGenBuffers(1, &_vbo));
    GL_CHECK(glGenBuffers(1, &_ebo));

    // MUST have a current context here.
    GL_CHECK(glBindVertexArray(_vao));

    // This must be the VBO that contains MeshVertex data
    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, _vbo));

    GL_CHECK(glEnableVertexAttribArray(0));
    GL_CHECK(glVertexAttribPointer(0,
                                   3,
                                   GL_FLOAT,
                                   GL_FALSE,
                                   MeshVertex::stride(),
                                   reinterpret_cast<void*>(0 * sizeof(float))));

    GL_CHECK(glEnableVertexAttribArray(1));
    GL_CHECK(glVertexAttribPointer(1,
                                   3,
                                   GL_FLOAT,
                                   GL_FALSE,
                                   MeshVertex::stride(),
                                   reinterpret_cast<void*>(3 * sizeof(float))));

    GL_CHECK(glEnableVertexAttribArray(2));
    GL_CHECK(glVertexAttribPointer(2,
                                   3,
                                   GL_FLOAT,
                                   GL_FALSE,
                                   MeshVertex::stride(),
                                   reinterpret_cast<void*>(6 * sizeof(float))));

    GL_CHECK(glEnableVertexAttribArray(3));
    GL_CHECK(glVertexAttribPointer(3,
                                   2,
                                   GL_FLOAT,
                                   GL_FALSE,
                                   MeshVertex::stride(),
                                   reinterpret_cast<void*>(9 * sizeof(float))));

    GL_CHECK(glBindVertexArray(0));
    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, 0));

    GLint vao = 0, vbo = 0;
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vao);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &vbo);
    Engine.logger.debug("attrib setup bindings: VAO={}, ARRAY_BUFFER={}", _vao, _vbo);

    _hasInitVertexAttributes = true;
    Engine.logger.debug("Vertex attributes initialized");
    return Failable::ok({});
}

Failable MeshBuffer::bindMeshData() {
    if (!_dataOutofDate) {
        return Failable::ok({});
    }

    Engine.logger.debug("Binding mesh data");

    if (!_hasInitVertexAttributes) {
        Failable r = initVertexAttributes();
        if (r.isError()) {
            return r;
        }
    }

    if (_vao == 0 || _vbo == 0 || _ebo == 0) {
        Engine.logger.error("Mesh data not initialized");
        return Failable::errorResult("Mesh data not initialized");
    }

    GL_CHECK(glBindVertexArray(_vao));

    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, _vbo));
    GL_CHECK(glBufferData(
        GL_ARRAY_BUFFER, _bufferData.size() * sizeof(GLfloat), _bufferData.data(), GL_STATIC_DRAW));

    GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo));
    GL_CHECK(glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                          _indices.size() * sizeof(GLuint),
                          _indices.data(),
                          GL_STATIC_DRAW));

    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, 0));

    Engine.logger.debug("Mesh data bound");
    _dataOutofDate = false;
    return Failable::ok({});
}

void MeshBuffer::drawMesh(const Mesh& mesh) {
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
