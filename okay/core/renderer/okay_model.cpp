#include "okay_model.hpp"

using namespace okay;

OkayModel OkayModelBuffer::AddModel(const OkayMeshData& mesh) {
    OkayModel m{};
    m.VertexOffset = Positions.size();
    m.VertexCount = mesh.Vertices.size();

    m.IndexOffset = Indices.size();
    m.IndexCount = mesh.Indices.size();

    // Reserve vertex attribute storage
    Positions.reserve(Positions.size() + m.VertexCount);
    Normals.reserve(Normals.size() + m.VertexCount);
    UVs.reserve(UVs.size() + m.VertexCount);
    Colors.reserve(Colors.size() + m.VertexCount);

    // Reserve index storage
    Indices.reserve(Indices.size() + m.IndexCount);

    // Append vertices (SoA)
    for (const OkayVertex& v : mesh.Vertices) {
        Positions.push_back(v.Position);
        Normals.push_back(v.Normal);
        UVs.push_back(v.UV);
        Colors.push_back(v.Color);
    }

    // Append indices, rebased to the global vertex offset
    const std::uint32_t base = static_cast<std::uint32_t>(m.VertexOffset);
    for (std::uint32_t i : mesh.Indices) {
        Indices.push_back(base + i);
    }

    return m;
}

OkayModelView OkayModelBuffer::GetModelView(OkayModel model) const {
    return OkayModelView(this, model);
}
