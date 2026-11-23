// Copyright (c) 2026 Okay Engine contributors
// SPDX-License-Identifier: MIT
//
// text_mesh_builder.hpp - Okay Engine

#pragma once

#include <okay/core/engine/engine.hpp>
#include <okay/core/renderer/mesh.hpp>
#include <okay/core/ui/text_layout.hpp>

#include <cstdint>
#include <string_view>
#include <vector>

namespace okay {

class TextMeshBuilder {
   public:
    static MeshData build(std::string_view text, const TextStyle& style, bool doubleSided);

   private:
    struct TextQuad {
        MeshVertex vertices[4];
    };

    static void appendQuadIndices(
        std::vector<std::uint32_t>& indices, std::uint32_t baseVertex, bool doubleSided);

    static TextQuad generateQuadForGlyph(
        const FontManager::Glyph& glyph, float baselineX, float baselineY, float layoutScale);
};

inline MeshData textMesh(std::string_view text, const TextStyle& style, bool doubleSided) {
    return TextMeshBuilder::build(text, style, doubleSided);
}

}  // namespace okay
