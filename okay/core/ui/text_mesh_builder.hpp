#ifndef __TEXT_MESH_BUILDER_H__
#define __TEXT_MESH_BUILDER_H__

#include <okay/core/engine/engine.hpp>
#include <okay/core/renderer/mesh.hpp>
#include <okay/core/ui/text_layout.hpp>

#include <cstdint>
#include <string_view>
#include <vector>

namespace okay {

class TextMeshBuilder {
   public:
    static Mesh build(std::string_view text,
                      const TextStyle& style,
                      MeshBuffer& buffer,
                      bool doubleSided);

   private:
    struct TextQuad {
        MeshVertex vertices[4];
    };

    static void appendQuadIndices(std::vector<std::uint32_t>& indices,
                                  std::uint32_t baseVertex,
                                  bool doubleSided);

    static TextQuad generateQuadForGlyph(const FontManager::Glyph& glyph,
                                         float baselineX,
                                         float baselineY,
                                         float layoutScale);
};

inline Mesh textMesh(std::string_view text,
                     const TextStyle& style,
                     bool doubleSided = false,
                     SystemParameter<Renderer> renderer = nullptr) {
    return TextMeshBuilder::build(text, style, renderer->meshBuffer(), doubleSided);
}

}  // namespace okay

#endif  // __TEXT_MESH_BUILDER_H__