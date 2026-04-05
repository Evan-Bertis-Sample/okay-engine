#ifndef __TEXT_MESH_BUILDER_H__
#define __TEXT_MESH_BUILDER_H__

#include <okay/core/renderer/mesh.hpp>
#include <okay/core/ui/text_layout.hpp>

#include <cstdint>
#include <string_view>
#include <vector>

namespace okay {

struct TextMeshBuildOptions {
    MeshBuffer& meshBuffer;
    bool doubleSided{false};
};

class TextMeshBuilder {
   public:
    static Mesh build(std::string_view text,
                      const TextStyle& style,
                      const TextMeshBuildOptions& options);

    static Mesh build(const TextLayout& layout, const TextMeshBuildOptions& options);

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

}  // namespace okay

#endif  // __TEXT_MESH_BUILDER_H__