#include "okay_mesh.hpp"

using namespace okay;

OkayMesh OkayMeshBuffer::addMesh(const OkayMeshData& mesh) {
    OkayMesh m{};

    // get the vertexOffset of the mesh
    m.vertexOffset = _bufferData.size() / OkayVertex::numFloats();
    m.vertexCount = mesh.vertices.size();

    m.indexOffset = _indices.size();
    m.indexCount = mesh.indices.size();

    // push the vertices of the mesh
    for (const OkayVertex &v : mesh.vertices) {
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
    _indices.erase(_indices.begin() + mesh.indexOffset, _indices.begin() + mesh.indexOffset + mesh.indexCount);
    // remove the vertices
    _bufferData.erase(_bufferData.begin() + mesh.vertexOffset * OkayVertex::numFloats(),
                      _bufferData.begin() + mesh.vertexOffset * OkayVertex::numFloats() + mesh.vertexCount * OkayVertex::numFloats());
}

OkayModelView OkayMeshBuffer::GetModelView(OkayMesh model) const {
    return OkayModelView(this, model);
}

Failable OkayMeshBuffer::initVertexAttributes() {
    // describe vertex attributes
    // in memory the layout of vertices will be
    // position, normal, color, uv

    // position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(OkayVertex), (void*)0);
    // normal
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(OkayVertex),
                          (void*)offsetof(OkayVertex, normal));

    // color
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(OkayVertex),
                          (void*)offsetof(OkayVertex, color));

    // uv
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(OkayVertex),
                          (void*)offsetof(OkayVertex, uv));

    return Failable::ok({});
}

Failable OkayMeshBuffer::bindMeshData() {
    // init the vao
    glGenVertexArrays(1, &_vao);
    glGenBuffers(1, &_vbo);

    glBindVertexArray(_vao);
    glBindBuffer(GL_ARRAY_BUFFER, _vbo);
    glBufferData(GL_ARRAY_BUFFER, _bufferData.size() * sizeof(GLfloat), &_bufferData[0], GL_STATIC_DRAW);

    // setup the vertex attributes 
    initVertexAttributes();

    // init the ebo
    glGenBuffers(1, &_ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, _indices.size() * sizeof(GLuint), &_indices[0], GL_STATIC_DRAW);

    // unbind
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    return Failable::ok({});
}