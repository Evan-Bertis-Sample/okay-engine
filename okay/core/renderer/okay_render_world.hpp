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

    OkayTransform() = default;

    OkayTransform(const glm::vec3& pos, const glm::vec3& scl, const glm::quat& rot)
        : position(pos), scale(scl), rotation(rot) {
    }

    // asignment, equality comparison overloads
    OkayTransform& operator=(const OkayTransform& other) {
        position = other.position;
        scale = other.scale;
        rotation = other.rotation;
        return *this;
    }

    bool operator==(const OkayTransform& other) const {
        return position == other.position && scale == other.scale && rotation == other.rotation;
    }

    bool operator!=(const OkayTransform& other) const {
        return !(*this == other);
    }

    glm::mat4 toMatrix() const {
        glm::mat4 mat(1.0f);
        mat = glm::translate(mat, position);
        mat *= glm::mat4_cast(rotation);
        mat = glm::scale(mat, scale);
        return mat;
    }
};

class OkayRenderWorld;

using RenderItemHandle = ObjectPoolHandle;

struct OkayRenderItem {
    IOkayMaterial* material{nullptr};
    OkayMesh mesh{};
    OkayTransform transform{};
    std::uint64_t sortKey{0};
    glm::mat4 worldMatrix{1.0f};

    RenderItemHandle parent{RenderItemHandle::invalidHandle()};
    RenderItemHandle firstChild{RenderItemHandle::invalidHandle()};
    RenderItemHandle previousSibling{RenderItemHandle::invalidHandle()};
    RenderItemHandle nextSibling{RenderItemHandle::invalidHandle()};

    OkayRenderItem() = default;

    OkayRenderItem(const IOkayMaterial* mat, OkayMesh m);
};

struct OkayRenderEntity {
   public:
   
   struct Properties {
       friend class OkayRenderEntity;
       friend class OkayRenderWorld;
       
        IOkayMaterial* material{nullptr};
        OkayTransform transform{};
        OkayMesh mesh{};

        ~Properties();
        
        Properties(const Properties&) = delete;
        Properties& operator=(const Properties&) = delete;
        Properties(Properties&&) noexcept = default;
        Properties& operator=(Properties&&) noexcept = default;

        Properties *operator->() {
            return this;
        }

        const Properties *operator->() const {
            return this;
        }

        private:
        RenderItemHandle _renderItem{RenderItemHandle::invalidHandle()};
        OkayRenderWorld& _owner;

        Properties(OkayRenderWorld& owner, RenderItemHandle renderItem)
            : _owner(owner), _renderItem(renderItem) {
        }
    };

    friend class OkayRenderWorld;
    
    OkayRenderEntity(OkayRenderWorld& owner, RenderItemHandle renderItem)
        : _owner(owner), _renderItem(renderItem) {
    }

    bool operator==(const OkayRenderEntity& other) const {
        return _renderItem == other._renderItem;
    }

    bool operator!=(const OkayRenderEntity& other) const {
        return !(*this == other);
    }

    Properties operator*() const;
    Properties operator->() const;

    Properties prop() const {
        return **this;
    }

   private:
    OkayRenderWorld& _owner;
    RenderItemHandle _renderItem{RenderItemHandle::invalidHandle()};
};

class OkayRenderWorld {
   public:
    struct ChildIterator {
       public:
        OkayRenderWorld& world;

        ChildIterator(OkayRenderWorld& world, RenderItemHandle h) : world(world), _renderItem(h) {
        }

        ChildIterator(const ChildIterator& other)
            : world(other.world), _renderItem(other._renderItem) {
        }

        ChildIterator& operator++();

        bool operator==(const ChildIterator& other) const;
        bool operator!=(const ChildIterator& other) const;

        OkayRenderEntity operator*() const;
        OkayRenderEntity operator->() const;

       private:
        RenderItemHandle _renderItem{RenderItemHandle::invalidHandle()};
    };

    struct ChildRange {
        ChildIterator b;
        ChildIterator e;

        ChildIterator begin() const {
            return b;
        }
        ChildIterator end() const {
            return e;
        }

        ChildRange(ChildIterator b, ChildIterator e) : b(b), e(e) {
        }
    };

    ChildRange children(OkayRenderEntity parent);
    const ChildRange children(OkayRenderEntity parent) const;

    OkayRenderEntity addRenderEntity(const OkayTransform& transform,
                                     IOkayMaterial* material,
                                     const OkayMesh& mesh,
                                     OkayRenderEntity parent);

    OkayRenderEntity addRenderEntity(const OkayTransform& transform,
                                     IOkayMaterial* material,
                                     const OkayMesh& mesh) {
        return addRenderEntity(
            transform, material, mesh, OkayRenderEntity(*this, RenderItemHandle::invalidHandle()));
    }

    const std::vector<const OkayRenderItem*>& getRenderItems();
    const OkayRenderItem& getRenderItem(RenderItemHandle handle) const {
        return _renderItemPool.get(handle);
    }

    OkayRenderEntity getRenderEntity(RenderItemHandle handle) {
        return OkayRenderEntity(*this, handle);
    }

    void updateEntity(RenderItemHandle renderItem, const Properties&& properties);

    Failable addChild(OkayRenderEntity parent, OkayRenderEntity children);
    bool isChildOf(OkayRenderEntity parent, OkayRenderEntity child) const;

   private:
    ObjectPool<OkayRenderItem> _renderItemPool;
    std::vector<const OkayRenderItem*> _memoizedRenderItems;
    std::set<RenderItemHandle> _dirtyEntities;
    bool _needsMaterialRebuild{true};

    void rebuildTransforms();
    void rebuildMaterials();

    void handleDirtyMesh(RenderItemHandle dirtyEntity);
    void handleDirtyMaterial(RenderItemHandle dirtyEntity);
    void handleDirtyTransform(RenderItemHandle dirtyEntity);
};

}  // namespace okay

#endif  // __OKAY_RENDER_WORLD_H__
