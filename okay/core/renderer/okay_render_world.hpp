#ifndef __OKAY_RENDER_WORLD_H__
#define __OKAY_RENDER_WORLD_H__

#include <cstdint>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <limits>
#include <okay/core/renderer/okay_material.hpp>
#include <okay/core/renderer/okay_mesh.hpp>
#include <okay/core/util/dirty_set.hpp>
#include <okay/core/util/object_pool.hpp>
#include <okay/core/util/property.hpp>
#include <vector>

namespace okay {

struct OkayTransform {
    glm::vec3 position{0.0f, 0.0f, 0.0f};
    glm::vec3 scale{1.0f, 1.0f, 1.0f};
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
};

struct OkayRenderItem;

using OkayRenderableHandle = ObjectPoolHandle;

struct OkayRenderItem {
   public:
    const IOkayMaterial& material;
    OkayMesh mesh;
    std::uint64_t sortKey{0};
    glm::mat4 worldMatrix{1.0f};

    OkayRenderItem(const IOkayMaterial& material, OkayMesh mesh) : material(material), mesh(mesh) {
        if (mesh.isEmpty()) {
            sortKey = std::numeric_limits<std::uint64_t>::max();
        }
        sortKey = (std::uint64_t(material.shaderID()) << 32 | std::uint64_t(material.id()));
    }

   private:
    OkayRenderableHandle parent{OkayRenderableHandle::invalidHandle()};
    OkayRenderableHandle firstChild{OkayRenderableHandle::invalidHandle()};
    OkayRenderableHandle previousSibling{OkayRenderableHandle::invalidHandle()};
    OkayRenderableHandle nextSibling{OkayRenderableHandle::invalidHandle()};
};

struct OkayRenderEntity {
   public:
    Property<IOkayMaterial&> material;
    Property<OkayTransform> transform;
    Property<OkayMesh> mesh;

   private:
    friend class OkayRenderWorld;

   public:
    OkayRenderWorld(Property<IOkayMaterial&> material, Property<OkayTransform> transform,
                    OkayMesh mesh)
        : material(material), transform(transform), mesh(mesh) {};
};

class OkayRenderWorld {
   public:
    OkayRenderableHandle addRenderEntity(const OkayTransform& transform, IOkayMaterial& material,
                                         const OkayMesh& mesh);

    const std::vector<const OkayRenderItem>& getRenderItems();

    // iterator for traversing the children of a render item
    class iterator {
    public:
        iterator(const OkayRenderWorld *world, OkayRenderableHandle handle) {
            _world = world;
            _handle = handle;
        }

        iterator& operator++() {
            _handle = _world->_renderItemPool.get(_handle).nextSibling;
            return *this;
        }

        iterator operator++(int) {
            iterator tmp = *this;
            ++*this;
            return tmp;
        }

        bool operator==(const iterator& other) const { return _handle == other._handle; }
        bool operator!=(const iterator& other) const { return _handle != other._handle; }

        const OkayRenderItem& operator*() const { return _world->_renderItemPool.get(_handle); }

        // begin and end iterators
        static iterator begin(const OkayRenderWorld* world) {
            return OkayRenderItemIterator(world, 0);
        }

        static iterator end(const OkayRenderWorld* world) {
            return iterator(world, OkayRenderableHandle::invalidHandle());
        }

    private:
        const OkayRenderWorld *_world{nullptr};
        OkayRenderableHandle _handle{OkayRenderableHandle::invalidHandle()};
    };

    iterator iterateChildren(OkayRenderableHandle handle) {
        return iterator(this, handle);
    }

   private:
    ObjectPool<OkayRenderEntity> _entityPool;
    ObjectPool<OkayRenderItem> _renderItemPool;
    std::vector<OkayRenderItem> _memoizedRenderEntities;

    struct RenderItemDirtyHandlerContext {
        OkayRenderWorld* world;
        OkayRenderableHandle handle;
    };

    static void handleDirtyTransform(void* ctx) {
        RenderItemDirtyHandlerContext* context = static_cast<RenderItemDirtyHandlerContext*>(ctx);
        OkayRenderableHandle handle = context->handle;
        // Do reupdate the tree

        OkayRenderItem item = context->world->_renderItemPool.get(handle);
        // add to the set of dirty transforms this, and all children
        std::set<OkayRenderableHandle> toProcess;
        toProcess.insert(handle);

        while (!toProcess.empty()) {
            OkayRenderableHandle current = *toProcess.begin();
            toProcess.erase(toProcess.begin());
            context->world->_dirtyTransforms.insert(current);

            OkayRenderItem& currentItem = context->world->_renderItemPool.get(current);
            for (auto childHandle : currentItem.children()) {
                toProcess.insert(childHandle);
            }
        }
    }

    static void handleDirtyMaterial(void* ctx) {
        RenderItemDirtyHandlerContext* context = static_cast<RenderItemDirtyHandlerContext*>(ctx);
        OkayRenderableHandle handle = context->handle;
        _needsMaterialRebuild = true;
    }

    static void handleDirtyMesh(void* ctx) {
        RenderItemDirtyHandlerContext* context = static_cast<RenderItemDirtyHandlerContext*>(ctx);
        OkayRenderableHandle handle = context->handle;
        // nothing for now, as mesh changes don't affect the memoized render items
    }

    void rebuildTransforms() {
        // for every dirty transform, recalculate world matrix
        for (OkayRenderableHandle handle : _dirtyTransforms) {
            OkayRenderItem& item = _renderItemPool.get(handle);
            OkayRenderEntity& entity = _entityPool.get(handle);

            glm::mat4 translationMat =
                glm::translate(glm::mat4(1.0f), entity.transform.get().position);
            glm::mat4 rotationMat = glm::mat4_cast(entity.transform.get().rotation);
            glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f), entity.transform.get().scale);

            item.worldMatrix = translationMat * rotationMat * scaleMat;
        }

        _dirtyTransforms.clear();
    }

    void rebuildMaterials() {
        // resort the the memoized render items
        _memoizedRenderEntities.clear();

        for (std::uint32_t i = 0; i < _entityPool.capacity(); ++i) {
            if (!_entityPool.aliveAt(i)) continue;
            OkayRenderableHandle handle{i, _entityPool.generationAt(i)};
            OkayRenderEntity& entity = _entityPool.get(handle);

            OkayRenderItem renderItem{entity.material.get(), entity.mesh.get()};
            OkayRenderableHandle renderItemHandle = _renderItemPool.emplace(std::move(renderItem));

            _memoizedRenderEntities.push_back(&_renderItemPool.get(renderItemHandle));
        }

        std::sort(_memoizedRenderEntities.begin(), _memoizedRenderEntities.end(),
                  [](const OkayRenderItem* a, const OkayRenderItem* b) {
                      return a->sortKey < b->sortKey;
                  });

        _needsMaterialRebuild = false;
    }

    std::set<OkayRenderableHandle> _dirtyTransforms;
    bool _needsMaterialRebuild{true};
};

}  // namespace okay

#endif  // __OKAY_RENDER_WORLD_H__
