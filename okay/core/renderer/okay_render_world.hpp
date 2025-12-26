#ifndef __OKAY_RENDER_WORLD_H__
#define __OKAY_RENDER_WORLD_H__

#include <algorithm>
#include <cstdint>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <limits>
#include <okay/core/renderer/okay_material.hpp>
#include <okay/core/renderer/okay_mesh.hpp>
#include <okay/core/util/dirty_set.hpp>
#include <okay/core/util/object_pool.hpp>
#include <okay/core/util/property.hpp>
#include <set>
#include <vector>

namespace okay {

struct OkayTransform {
    glm::vec3 position{0.0f, 0.0f, 0.0f};
    glm::vec3 scale{1.0f, 1.0f, 1.0f};
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
};

class OkayRenderWorld;

using EntityHandle = ObjectPoolHandle;
using ItemHandle = ObjectPoolHandle;

struct OkayRenderItem {
    const IOkayMaterial* material{nullptr};
    OkayMesh mesh{};
    std::uint64_t sortKey{0};
    glm::mat4 worldMatrix{1.0f};

    ItemHandle parent{ItemHandle::invalidHandle()};
    ItemHandle firstChild{ItemHandle::invalidHandle()};
    ItemHandle previousSibling{ItemHandle::invalidHandle()};
    ItemHandle nextSibling{ItemHandle::invalidHandle()};

    OkayRenderItem() = default;

    OkayRenderItem(const IOkayMaterial* mat, OkayMesh m);
};

struct OkayRenderEntity {
    Property<IOkayMaterial*> material;
    Property<OkayTransform> transform;
    Property<OkayMesh> mesh;

    ItemHandle renderItem{ItemHandle::invalidHandle()};

    OkayRenderEntity(Property<IOkayMaterial*> material, Property<OkayTransform> transform,
                     Property<OkayMesh> mesh)
        : material(material), transform(transform), mesh(mesh) {}
};

class OkayRenderWorld {
   public:
    struct ChildIterator {
        const OkayRenderWorld* world{nullptr};
        ItemHandle handle{ItemHandle::invalidHandle()};

        ChildIterator() = default;
        ChildIterator(const OkayRenderWorld* w, ItemHandle h) : world(w), handle(h) {}

        ChildIterator& operator++();

        bool operator==(const ChildIterator& other) const;
        bool operator!=(const ChildIterator& other) const;

        const OkayRenderItem& operator*() const;
        const OkayRenderItem* operator->() const;
    };

    struct ChildRange {
        ChildIterator b;
        ChildIterator e;

        ChildIterator begin() const { return b; }
        ChildIterator end() const { return e; }
    };

    ChildRange children(ItemHandle parent) const;

    EntityHandle addRenderEntity(const OkayTransform& transform, IOkayMaterial* material,
                                 const OkayMesh& mesh);

    const std::vector<const OkayRenderItem*>& getRenderItems();

   private:
    ObjectPool<OkayRenderEntity> _entityPool;
    ObjectPool<OkayRenderItem> _renderItemPool;

    std::vector<const OkayRenderItem*> _memoizedRenderItems;

    struct DirtyContext {
        OkayRenderWorld* world{nullptr};
        EntityHandle entity{EntityHandle::invalidHandle()};
    };

    static void handleDirtyTransform(void* ctx);
    static void handleDirtyMaterial(void* ctx);
    static void handleDirtyMesh(void* ctx);
    void rebuildTransforms();
    void rebuildMaterials();

    std::set<EntityHandle> _dirtyEntities;
    bool _needsMaterialRebuild{true};
};

}  // namespace okay

#endif  // __OKAY_RENDER_WORLD_H__
