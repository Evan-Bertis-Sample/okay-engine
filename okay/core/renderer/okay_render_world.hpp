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
#include <set>
#include <span>
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

    struct ChildrenView {
        OkayRenderItem* item;
        OkayRenderableHandle current;

        struct iterator {
           public:
            using value_type = OkayRenderItem&;
            using difference_type = std::ptrdiff_t;
            using iterator_category = std::forward_iterator_tag;

            iterator(OkayRenderItem* parent, OkayRenderableHandle handle)
                : _parent(parent), _current(handle) {}

            OkayRenderItem& operator*() const {
                return _parent->world->_renderItemPool.get(_current);
            }

            iterator& operator++() {
                OkayRenderItem& currentItem = _parent->world->_renderItemPool.get(_current);
                _current = currentItem.nextSibling;
                return *this;
            }

            bool operator!=(const iterator& other) const { return _current != other._current; }

           private:
            OkayRenderItem* _parent;
            OkayRenderableHandle _current;
        };

        iterator begin() { return iterator(item, item->firstChild); }
        iterator end() { return iterator(item, OkayRenderableHandle::invalidHandle()); }
    };

    ChildrenView children() { return ChildrenView{this, firstChild}; }

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
                                         const OkayMesh& mesh) {
        OkayRenderableHandle handle = _entityPool.emplace(material, mesh);

        // Create properties
        RenderItemDirtyHandlerContext context{this, handle};

        Property<OkayTransform> transformProperty(transform);
        transformProperty.bind(Property<OkayTransform>::Delegate::fromFunction(
            &OkayRenderWorld::handleDirtyTransform, &context));

        Property<IOkayMaterial&> materialProperty(material);
        materialProperty.bind(Property<IOkayMaterial&>::Delegate::fromFunction(
            &OkayRenderWorld::handleDirtyMaterial, &context));

        Property<OkayMesh> meshProperty(mesh);
        meshProperty.bind(Property<OkayMesh>::Delegate::fromFunction(
            &OkayRenderWorld::handleDirtyMesh, &context));

        OkayRenderEntity& entity = _entityPool.get(handle);
        entity.transform = std::move(transformProperty);
        entity.material = std::move(materialProperty);
        entity.mesh = std::move(meshProperty);

        // add to dirty transforms and mark for rebuild
        context->world->_dirtyTransforms.insert(handle);
        context->world->_needsMaterialRebuild = true;

        return handle;
    };

    const std::vector<const OkayRenderItem>& getRenderItems() {
        if (!_dirtyTransforms.empty()) {
            rebuildTransforms();
        }

        if (_needsMaterialRebuild) {
            rebuildMaterials();
        }

        return _memoizedRenderEntities;
    }

   private:
    ObjectPool<OkayRenderEntity> _entityPool;
    ObjectPool<OkayRenderItem> _renderItemPool;
    std::vector<const OkayRenderItem> _memoizedRenderEntities;

    struct RenderItemDirtyHandlerContext {
        OkayRenderWorld *world;
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

            _memoizedRenderEntities.push_back(
                &_renderItemPool.get(renderItemHandle));
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
