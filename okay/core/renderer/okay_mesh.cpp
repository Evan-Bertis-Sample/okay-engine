#include "okay_mesh.hpp"

using namespace okay;

OkayMesh OkayMeshBuffer::AddModel(const OkayMeshData& mesh) {
    OkayMesh m{};
    m.vertexOffset = positions.size();
    m.vertexCount = mesh.vertices.size();

    m.indexOffset = indices.size();
    m.indexCount = mesh.indices.size();

    // Reserve vertex attribute storage
    positions.reserve(positions.size() + m.vertexCount);
    normals.reserve(normals.size() + m.vertexCount);
    uvs.reserve(uvs.size() + m.vertexCount);
    colors.reserve(colors.size() + m.vertexCount);

    // Reserve index storage
    indices.reserve(indices.size() + m.indexCount);

    // Append vertices (SoA)
    for (const OkayVertex& v : mesh.vertices) {
        positions.push_back(v.position);
        normals.push_back(v.normal);
        uvs.push_back(v.uv);
        colors.push_back(v.color);
    }

    // Append indices, rebased to the global vertex offset
    const std::uint32_t base = static_cast<std::uint32_t>(m.vertexOffset);
    for (std::uint32_t i : mesh.indices) {
        indices.push_back(base + i);
    }

    return m;
}

OkayModelView OkayMeshBuffer::GetModelView(OkayMesh model) const {
    return OkayModelView(this, model);
}
