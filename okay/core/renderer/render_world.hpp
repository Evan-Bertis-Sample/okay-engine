#ifndef _RENDER_WORLD_H__
#define _RENDER_WORLD_H__

#include "glm/ext/vector_float3.hpp"
#include "glm/ext/vector_float4.hpp"

#include <okay/core/renderer/material.hpp>
#include <okay/core/renderer/mesh.hpp>
#include <okay/core/util/dirty_set.hpp>
#include <okay/core/util/object_pool.hpp>
#include <okay/core/util/property.hpp>
#include <okay/core/renderer/gl.hpp>

#include <cstdint>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <set>
#include <variant>
#include <vector>

namespace okay {

struct Transform {
    glm::vec3 position{0.0f, 0.0f, 0.0f};
    glm::vec3 scale{1.0f, 1.0f, 1.0f};
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};

    Transform(const glm::vec3& pos = glm::vec3(0.0f),
                  const glm::vec3& scl = glm::vec3(1.0f),
                  const glm::quat& rot = glm::quat())
        : position(pos), scale(scl), rotation(rot) {}

    // asignment, equality comparison overloads
    Transform& operator=(const Transform& other) {
        position = other.position;
        scale = other.scale;
        rotation = other.rotation;
        return *this;
    }

    bool operator==(const Transform& other) const {
        return position == other.position && scale == other.scale && rotation == other.rotation;
    }

    bool operator!=(const Transform& other) const { return !(*this == other); }

    glm::mat4 toMatrix() const {
        glm::mat4 mat(1.0f);
        mat = glm::translate(mat, position);
        mat *= glm::mat4_cast(rotation);
        mat = glm::scale(mat, scale);
        return mat;
    }
};

class Camera {
   public:
    enum class ProjectionType { PERPSECTIVE, ORTHOGRAPHIC };

    struct Perspective {
        float fov{45.0f};
        float near{0.1f};
        float far{1000.0f};
    };

    struct OrthographicConfig {
        float left{-1.0f};
        float right{1.0f};
        float bottom{-1.0f};
        float top{1.0f};
        float near{0.1f};
        float far{1000.0f};
    };

    Transform transform{};

    Camera() = default;

    template <typename T>
        requires(std::is_same_v<T, Perspective> || std::is_same_v<T, OrthographicConfig>)
    void setConfig(const T& config) {
        if constexpr (std::is_same_v<T, Perspective>) {
            _projectionType = ProjectionType::PERPSECTIVE;
        } else {
            _projectionType = ProjectionType::ORTHOGRAPHIC;
        }
        _config = config;
    }

    template <typename T>
        requires(std::is_same_v<T, Perspective> || std::is_same_v<T, OrthographicConfig>)
    Option<T&> getConfig() {
        if (auto* p = std::get_if<T>(&_config)) {
            return *p;
        }
        return Option<T&>::none();
    }

    template <typename T>
        requires(std::is_same_v<T, Perspective> || std::is_same_v<T, OrthographicConfig>)
    Option<const T&> getConfig() const {
        if (auto* p = std::get_if<T>(&_config)) {
            return *p;
        }
        return Option<const T&>::none();
    }

    glm::mat4 projectionMatrix(float ratio) const {
        if (_projectionType == ProjectionType::PERPSECTIVE) {
            Perspective config = std::get<Perspective>(_config);
            return glm::perspective(glm::radians(config.fov), ratio, config.near, config.far);
        } else {
            OrthographicConfig config = std::get<OrthographicConfig>(_config);
            return glm::ortho(
                config.left, config.right, config.bottom, config.top, config.near, config.far);
        }
    }

    glm::mat4 viewMatrix() const { return glm::inverse(transform.toMatrix()); }

    void lookAt(glm::vec3 center, glm::vec3 up = glm::vec3(0, 1, 0)) {
        glm::vec3 eye = transform.position;
        glm::vec3 f = glm::normalize(center - eye);       // desired forward (toward target)
        glm::vec3 r = glm::normalize(glm::cross(f, up));  // right
        glm::vec3 u = glm::cross(r, f);                   // corrected up

        // Camera local axes in world space:
        // +X = r, +Y = u, -Z = f  => +Z = -f
        glm::mat3 worldRot(r, u, -f);  // columns

        transform.rotation = glm::quat_cast(worldRot);
    }

    glm::vec3 position() const { return transform.position; }
    glm::vec3 direction() const { return transform.rotation * glm::vec3(0, 0, -1); }

   private:
    ProjectionType _projectionType{ProjectionType::PERPSECTIVE};
    std::variant<Perspective, OrthographicConfig> _config{Perspective{}};
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
};

class RenderWorld;

using RenderItemHandle = ObjectPoolHandle;

struct RenderItem {
    MaterialHandle material;
    Mesh mesh;
    Transform transform;
    std::uint64_t sortKey;
    glm::mat4 worldMatrix{1.0f};

    RenderItemHandle parent{RenderItemHandle::invalidHandle()};
    RenderItemHandle firstChild{RenderItemHandle::invalidHandle()};
    RenderItemHandle previousSibling{RenderItemHandle::invalidHandle()};
    RenderItemHandle nextSibling{RenderItemHandle::invalidHandle()};

    RenderItem() = default;
    RenderItem(MaterialHandle mat, Mesh m);

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

    bool operator==(const RenderEntity& other) const {
        return _renderItem == other._renderItem;
    }

    bool operator!=(const RenderEntity& other) const { return !(*this == other); }

    Properties operator*() const;
    Properties operator->() const;

    Properties prop() const { return **this; }

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

    const std::vector<RenderItemHandle>& getRenderItems();
    const RenderItem& getRenderItem(RenderItemHandle handle) const {
        return _renderItemPool.get(handle);
    }

    RenderItem& getRenderItem(RenderItemHandle handle) { return _renderItemPool.get(handle); }

    RenderEntity getRenderEntity(RenderItemHandle handle) {
        return RenderEntity(this, handle);
    }

    void updateEntity(RenderItemHandle renderItem, const RenderEntity::Properties&& properties);

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
