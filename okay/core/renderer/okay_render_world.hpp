#ifndef __OKAY_RENDER_WORLD_H__
#define __OKAY_RENDER_WORLD_H__

#include <cstdint>
#include <span>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <okay/core/renderer/okay_material.hpp>
#include <okay/core/renderer/okay_mesh.hpp>

#include <okay/core/util/object_pool.hpp>
#include <okay/core/util/dirty_set.hpp>

namespace okay {

struct OkayTransform {
    glm::vec3 position{0.0f, 0.0f, 0.0f};
    glm::vec3 scale{1.0f, 1.0f, 1.0f};
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
};

struct OkayDrawItem {
    glm::mat4 worldFromMesh{1.0f};
    OkayMesh mesh{};
    OkayMaterial material{};
    std::uint64_t sortKey{0};
    std::uint32_t entityIndex{0xFFFFFFFFu};
};

class ChildrenView final {
   public:
    using iterator = std::vector<std::uint32_t>::const_iterator;

    ChildrenView() = default;
    explicit ChildrenView(std::vector<std::uint32_t> v) : _v(std::move(v)) {}

    iterator begin() const { return _v.begin(); }
    iterator end() const { return _v.end(); }

    std::size_t size() const { return _v.size(); }
    bool empty() const { return _v.empty(); }

    std::uint32_t operator[](std::size_t i) const { return _v[i]; }

    std::span<const std::uint32_t> span() const { return std::span<const std::uint32_t>(_v.data(), _v.size()); }

   private:
    std::vector<std::uint32_t> _v;
};

class SceneHierarchy final {
   public:
    struct Node {
        std::uint32_t parent{0xFFFFFFFFu};
        std::uint32_t firstChild{0xFFFFFFFFu};
        std::uint32_t nextSibling{0xFFFFFFFFu};
        std::uint32_t prevSibling{0xFFFFFFFFu};
    };

    static constexpr std::uint32_t NullIndex = 0xFFFFFFFFu;

    SceneHierarchy() = default;

    void resize(std::size_t n);

    std::uint32_t parentOf(std::uint32_t idx) const;
    std::uint32_t firstChildOf(std::uint32_t idx) const;
    std::uint32_t nextSiblingOf(std::uint32_t idx) const;
    std::uint32_t prevSiblingOf(std::uint32_t idx) const;

    void attach(std::uint32_t child, std::uint32_t parent);
    void detach(std::uint32_t child);

    ChildrenView children(std::uint32_t parent) const;

    void onDestroy(std::uint32_t idx);

   private:
    std::vector<Node> _nodes;
};

class OkayRenderWorld final {
   public:
    using Entity = ObjectPool<struct Slot>::Handle;
    static constexpr std::uint32_t NullIndex = 0xFFFFFFFFu;
    static constexpr Entity NullEntity{NullIndex, 0u};

    OkayRenderWorld() = default;
    ~OkayRenderWorld() = default;

    OkayRenderWorld(const OkayRenderWorld&) = delete;
    OkayRenderWorld& operator=(const OkayRenderWorld&) = delete;

    OkayRenderWorld(OkayRenderWorld&&) = default;
    OkayRenderWorld& operator=(OkayRenderWorld&&) = default;

    Entity spawn(const OkayTransform& localTransform, const OkayMesh& mesh, const OkayMaterial& material,
                 Entity parent = NullEntity);

    void destroy(Entity e);

    bool valid(Entity e) const { return _pool.valid(e); }
    std::size_t size() const { return _pool.size(); }

    Entity parentOf(Entity e) const;
    ChildrenView children(Entity e) const;

    void attach(Entity child, Entity newParent);
    void detach(Entity child);

    const OkayTransform& localTransform(Entity e) const;
    const glm::mat4& worldFromEntity(Entity e) const;

    const OkayMesh& mesh(Entity e) const;
    const OkayMaterial& material(Entity e) const;

    void setTransform(Entity e, const OkayTransform& t);
    void setMesh(Entity e, const OkayMesh& mesh);
    void setMaterial(Entity e, const OkayMaterial& material);

    template <class Fn>
    void withTransform(Entity e, Fn&& fn) {
        auto& t = transformRef(e);
        fn(t);
        markTransformDirty(e.index);
    }

    template <class Fn>
    void withRenderable(Entity e, Fn&& fn) {
        auto& m = meshRef(e);
        auto& mat = materialRef(e);
        fn(m, mat);
        markRenderablesDirty();
    }

    std::span<const OkayDrawItem> drawItems();

    void invalidateAll();

   private:
    struct Slot {
        OkayTransform local{};
        glm::mat4 world{1.0f};
        OkayMesh mesh{};
        OkayMaterial material{};
    };

    Slot& slotRef(Entity e);
    const Slot& slotRef(Entity e) const;

    OkayTransform& transformRef(Entity e);
    OkayMesh& meshRef(Entity e);
    OkayMaterial& materialRef(Entity e);

    void ensureIndexCapacity(std::uint32_t index);

    void markTransformDirty(std::uint32_t index);
    void markSubtreeTransformDirty(std::uint32_t index);

    void processDirtyTransforms();
    void updateWorldSubtree(std::uint32_t rootIndex, const glm::mat4& parentWorld);

    void markRenderablesDirty() { _renderablesDirty = true; }

    void rebuildDrawList();
    void syncDrawItemTransform(std::uint32_t index);

    static glm::mat4 compose(const OkayTransform& t);
    static std::uint64_t makeSortKey(const OkayMaterial& material, const OkayMesh& mesh);

   private:
    ObjectPool<Slot> _pool;
    SceneHierarchy _hierarchy;

    DirtySet<std::uint32_t> _dirtyTransformRoots;

    bool _renderablesDirty{true};

    std::vector<OkayDrawItem> _drawItems;
    std::vector<std::uint32_t> _entityToDrawIndex;
};

}  // namespace okay

#endif  // __OKAY_RENDER_WORLD_H__
