#ifndef __RENDER_WORLD_H__
#define __RENDER_WORLD_H__

#include "glm/ext/vector_float3.hpp"
#include "glm/ext/vector_float4.hpp"
#include "math_types.hpp"

#include <okay/core/renderer/gl.hpp>
#include <okay/core/renderer/material.hpp>
#include <okay/core/renderer/mesh.hpp>
#include <okay/core/util/dirty_set.hpp>
#include <okay/core/util/object_pool.hpp>
#include <okay/core/util/property.hpp>

#include <cstdint>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <set>
#include <variant>
#include <vector>

namespace okay {

class Camera {
   public:
    enum class ProjectionType { PERPSECTIVE, ORTHOGRAPHIC };

    struct PerspectiveLens {
        float fov{45.0f};
        float near{0.1f};
        float far{1000.0f};

        bool operator==(const PerspectiveLens& other) const {
            return fov == other.fov && near == other.near && far == other.far;
        }

        bool operator!=(const PerspectiveLens& other) const { return !(*this == other); }
    };

    struct OrthographicLens {
        float left{-1.0f};
        float right{1.0f};
        float bottom{-1.0f};
        float top{1.0f};
        float near{0.1f};
        float far{1000.0f};

        bool operator==(const OrthographicLens& other) const {
            return left == other.left && right == other.right && bottom == other.bottom &&
                   top == other.top && near == other.near && far == other.far;
        }

        bool operator!=(const OrthographicLens& other) const { return !(*this == other); }
    };

    using Lens = std::variant<PerspectiveLens, OrthographicLens>;

    Transform transform{};
    Lens lens{PerspectiveLens{}};

    ProjectionType projectionType() const {
        return std::holds_alternative<PerspectiveLens>(lens) ? ProjectionType::PERPSECTIVE
                                                             : ProjectionType::ORTHOGRAPHIC;
    }

    glm::mat4 projectionMatrix(float aspectRatio) const {
        if (projectionType() == ProjectionType::PERPSECTIVE) {
            const PerspectiveLens& p = std::get<PerspectiveLens>(lens);
            return glm::perspective(glm::radians(p.fov), aspectRatio, p.near, p.far);
        } else {
            const OrthographicLens& o = std::get<OrthographicLens>(lens);
            return glm::ortho(o.left, o.right, o.bottom, o.top, o.near, o.far);
        }
    }

    glm::mat4 viewMatrix() const { return glm::inverse(transform.toMatrix()); }
    glm::vec3 position() const { return transform.position; }
    glm::vec3 direction() const { return transform.rotation * glm::vec3(0, 0, -1); }

    bool isInFrustum(const glm::vec3& pos, float aspectRatio) const {
        glm::vec4 clip = projectionMatrix(aspectRatio) * viewMatrix() * glm::vec4(pos, 1.0f);

        return clip.x >= -clip.w && clip.x <= clip.w && clip.y >= -clip.w && clip.y <= clip.w &&
               clip.z >= -clip.w && clip.z <= clip.w;
    }

    bool isInFrustum(const Bounds& bounds, float aspectRatio) {
        for (const glm::vec3& p : bounds.corners()) {
            if (!isInFrustum(p, aspectRatio))
                return false;
        }
        return true;
    }

    operator Transform() const { return transform; }
    bool operator==(const Camera& other) const {
        return transform == other.transform && lens == other.lens;
    }

    bool operator!=(const Camera& other) const { return !(*this == other); }
};

struct alignas(16) Light {
    // xyz = position (world), w = type
    glm::vec4 posType;
    // rgb = color, w = intensity
    glm::vec4 color;
    // xyz = direction
    glm::vec4 direction;
    // directional : unused
    // point: radius, radius, radius, radius
    // spot : radius, angle, unused unused
    glm::vec4 extra;

    enum class Type : int { DIRECTIONAL = 0, POINT = 1, SPOT = 2 };

    static Light directional(glm::vec3 dir, glm::vec3 rgb, float intensity = 1.0f) {
        Light l{};
        l.posType = glm::vec4(dir, Type::DIRECTIONAL);
        l.color = glm::vec4(rgb, intensity);
        l.direction = glm::vec4(dir, 0.0f);
        return l;
    }

    static Light point(glm::vec3 pos, float radius, glm::vec3 rgb, float intensity = 1.0f) {
        Light l{};
        l.posType = glm::vec4(pos, Type::POINT);
        l.color = glm::vec4(rgb, intensity);
        l.extra = glm::vec4(radius);
        return l;
    }

    static Light spot(glm::vec3 pos,
                      glm::vec3 dir,
                      float radius,
                      float angleRad,
                      glm::vec3 rgb,
                      float intensity = 1.0f) {
        Light l{};
        l.posType = glm::vec4(pos, Type::SPOT);
        l.color = glm::vec4(rgb, intensity);
        l.direction = glm::vec4(dir, 0.0f);
        l.extra = glm::vec4(radius, angleRad, 0.0f, 0.0f);
        return l;
    }

    Type type() const { return static_cast<Type>((int)posType.w); }

    void setPosition(glm::vec3 pos) { posType = glm::vec4(pos, posType.w); }

    static constexpr std::size_t MAX_LIGHTS = 16;

    bool operator==(const Light& other) const {
        return posType == other.posType && color == other.color && direction == other.direction &&
               extra == other.extra;
    }

    bool operator!=(const Light& other) const { return !(*this == other); }
};

class RenderWorld;

using RenderItemHandle = ObjectPoolHandle;

struct RenderItem {
    MaterialHandle material;
    Mesh mesh;
    Transform transform;
    std::uint8_t renderLayer;
    std::uint64_t sortKey;
    glm::mat4 worldMatrix{1.0f};

    RenderItemHandle parent{RenderItemHandle::invalidHandle()};
    RenderItemHandle firstChild{RenderItemHandle::invalidHandle()};
    RenderItemHandle previousSibling{RenderItemHandle::invalidHandle()};
    RenderItemHandle nextSibling{RenderItemHandle::invalidHandle()};

    RenderItem() = default;
    RenderItem(MaterialHandle mat, Mesh m);

    void computeSortKey();

    // operator overloads for std::map
    bool operator<(const RenderItem& other) const { return sortKey < other.sortKey; }
    bool operator>(const RenderItem& other) const { return sortKey > other.sortKey; }
    bool operator==(const RenderItem& other) const { return sortKey == other.sortKey; }
    bool operator!=(const RenderItem& other) const { return !(*this == other); }
    bool operator<=(const RenderItem& other) const { return sortKey <= other.sortKey; }
    bool operator>=(const RenderItem& other) const { return sortKey >= other.sortKey; }
};

struct RenderEntity {
   public:
    struct Properties {
        friend class RenderEntity;
        friend class RenderWorld;

        MaterialHandle material{};
        Transform transform{};
        Mesh mesh{};
        std::uint8_t renderLayer{0};

        ~Properties();
        Properties* operator->() { return this; }
        const Properties* operator->() const { return this; }

       private:
        RenderItemHandle _renderItem{RenderItemHandle::invalidHandle()};
        RenderWorld* _owner;

        Properties(RenderWorld* owner, RenderItemHandle renderItem)
            : _owner(owner), _renderItem(renderItem) {}
    };

    friend class RenderWorld;

    constexpr RenderEntity() = default;

    RenderEntity(RenderWorld* owner, RenderItemHandle renderItem)
        : _owner(owner), _renderItem(renderItem) {}

    bool operator==(const RenderEntity& other) const { return _renderItem == other._renderItem; }
    bool operator!=(const RenderEntity& other) const { return !(*this == other); }
    Properties operator*() const;
    Properties operator->() const;

    Properties prop() const { return **this; }
    bool isValid() const;

   private:
    RenderWorld* _owner{nullptr};
    RenderItemHandle _renderItem{RenderItemHandle::invalidHandle()};
};

class RenderWorld {
   public:
    struct ChildIterator {
       public:
        RenderWorld& world;

        ChildIterator(RenderWorld& world, RenderItemHandle h) : world(world), _renderItem(h) {}

        ChildIterator(const ChildIterator& other)
            : world(other.world), _renderItem(other._renderItem) {}

        ChildIterator& operator++();

        bool operator==(const ChildIterator& other) const;
        bool operator!=(const ChildIterator& other) const;

        RenderEntity operator*() const;
        RenderEntity operator->() const;

       private:
        RenderItemHandle _renderItem{RenderItemHandle::invalidHandle()};
    };

    struct ChildRange {
        ChildIterator b;
        ChildIterator e;

        ChildIterator begin() const { return b; }
        ChildIterator end() const { return e; }

        ChildRange(ChildIterator b, ChildIterator e) : b(b), e(e) {}
    };

    Camera& camera() { return _camera; }

    ChildRange children(RenderEntity parent);
    const ChildRange children(RenderEntity parent) const;

    RenderEntity addRenderEntity(const Transform& transform,
                                 const MaterialHandle& material,
                                 const Mesh& mesh,
                                 RenderEntity parent);

    RenderEntity addRenderEntity(const Transform& transform,
                                 const MaterialHandle& material,
                                 const Mesh& mesh) {
        return addRenderEntity(
            transform, material, mesh, RenderEntity(this, RenderItemHandle::invalidHandle()));
    }

    const std::span<RenderItemHandle> getRenderItems();
    const RenderItem& getRenderItem(RenderItemHandle handle) const {
        return _renderItemPool.get(handle);
    }

    RenderItem& getRenderItem(RenderItemHandle handle) { return _renderItemPool.get(handle); }
    RenderEntity getRenderEntity(RenderItemHandle handle) { return RenderEntity(this, handle); }
    void updateEntity(RenderItemHandle renderItem, const RenderEntity::Properties&& properties);
    void removeRenderEntity(RenderEntity entity) { removeRenderEntity(entity._renderItem); }
    void removeRenderEntity(RenderItemHandle renderItem);
    bool isValidEntity(const RenderItemHandle& renderItem) const {
        return _renderItemPool.valid(renderItem);
    }
    bool isValidEntity(const RenderEntity& entity) const {
        return _renderItemPool.valid(entity._renderItem);
    }
    std::size_t numRenderItems() const { return _activeRenderItems; }

    Failable addChild(RenderEntity parent, RenderEntity children);
    bool isChildOf(RenderEntity parent, RenderEntity child) const;

    std::span<const Light> lights() const {
        return std::span<const Light>(_lights.data(), _activeLights);
    }

    std::size_t addLight(const Light& light) {
        _lights[_activeLights++] = light;
        return _activeLights - 1;
    }

    void removeLight(std::size_t index) {
        // swap the light with the last one
        _lights[index] = _lights[--_activeLights];
    }

    Light& getLight(std::size_t index) { return _lights[index]; }

   private:
    ObjectPool<RenderItem> _renderItemPool;
    std::vector<RenderItemHandle> _memoizedRenderItems;
    std::set<RenderItemHandle> _dirtyTransforms;
    std::size_t _activeRenderItems{0};

    std::array<Light, Light::MAX_LIGHTS> _lights{};
    std::size_t _activeLights{0};

    bool _needsMaterialRebuild{true};
    Camera _camera;

    void rebuildTransforms();
    void rebuildMaterials();

    void handleDirtyMesh(RenderItemHandle dirtyEntity);
    void handleDirtyMaterial(RenderItemHandle dirtyEntity);
    void handleDirtyTransform(RenderItemHandle dirtyEntity);
};

}  // namespace okay

#endif  // _RENDER_WORLD_H__
