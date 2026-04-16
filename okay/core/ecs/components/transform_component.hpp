#ifndef __TRANSFORM_COMPONENT_H__
#define __TRANSFORM_COMPONENT_H__

#include <okay/core/ecs/ecs.hpp>
#include <okay/core/renderer/render_world.hpp>

#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace okay {

struct TransformComponent {
    Transform transform;

    TransformComponent(const glm::vec3& position = glm::vec3(0.0f),
                       const glm::vec3& scale = glm::vec3(1.0f),
                       const glm::quat& rotation = glm::quat())
        : transform(position, scale, rotation) {}
    TransformComponent(const Transform& transform) : transform(transform) {}

    glm::mat4 toMatrix() const { return transform.toMatrix(); }

    glm::vec3 localForward() const {
        return glm::normalize(transform.rotation * glm::vec3(0.0f, 0.0f, -1.0f));
    }

    glm::vec3 localUp() const {
        return glm::normalize(transform.rotation * glm::vec3(0.0f, 1.0f, 0.0f));
    }

    glm::vec3 localRight() const {
        return glm::normalize(transform.rotation * glm::vec3(1.0f, 0.0f, 0.0f));
    }

    void setLocalDirection(glm::vec3 direction, glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f)) {
        if (glm::length(direction) < 1e-8f) {
            return;
        }

        glm::vec3 f = glm::normalize(direction);
        glm::vec3 upNorm = glm::normalize(up);

        if (std::abs(glm::dot(f, upNorm)) > 0.999f) {
            upNorm = (std::abs(f.y) < 0.999f) ? glm::vec3(0.0f, 1.0f, 0.0f)
                                              : glm::vec3(1.0f, 0.0f, 0.0f);
        }

        glm::vec3 r = glm::normalize(glm::cross(f, upNorm));
        glm::vec3 u = glm::normalize(glm::cross(r, f));

        transform.rotation = glm::quat_cast(glm::mat3(r, u, -f));
    }

    glm::mat4 getWorldMatrix(const ECSEntity& entity) const {
        glm::mat4 worldMatrix = transform.toMatrix();
        ECSEntity currentEntity = entity.getParent();

        while (currentEntity.isValid() && currentEntity.hasComponent<TransformComponent>()) {
            const TransformComponent& parentTransform =
                currentEntity.getComponent<TransformComponent>().value();
            worldMatrix = parentTransform.transform.toMatrix() * worldMatrix;
            currentEntity = currentEntity.getParent();
        }

        return worldMatrix;
    }

    glm::vec3 forward(const ECSEntity& entity) const {
        return glm::normalize(getWorldRotation(entity) * glm::vec3(0.0f, 0.0f, -1.0f));
    }

    glm::vec3 up(const ECSEntity& entity) const {
        return glm::normalize(getWorldRotation(entity) * glm::vec3(0.0f, 1.0f, 0.0f));
    }

    glm::vec3 right(const ECSEntity& entity) const {
        return glm::normalize(getWorldRotation(entity) * glm::vec3(1.0f, 0.0f, 0.0f));
    }

    glm::vec3 getWorldPosition(const ECSEntity& entity) const {
        glm::mat4 worldMatrix = getWorldMatrix(entity);
        return glm::vec3(worldMatrix[3]);
    }

    glm::vec3 getWorldScale(const ECSEntity& entity) const {
        glm::mat4 worldMatrix = getWorldMatrix(entity);
        return glm::vec3(glm::length(glm::vec3(worldMatrix[0])),
                         glm::length(glm::vec3(worldMatrix[1])),
                         glm::length(glm::vec3(worldMatrix[2])));
    }

    glm::quat getWorldRotation(const ECSEntity& entity) const {
        glm::quat worldRotation = transform.rotation;
        ECSEntity currentEntity = entity.getParent();

        while (currentEntity.isValid() && currentEntity.hasComponent<TransformComponent>()) {
            const TransformComponent& parentTransform =
                currentEntity.getComponent<TransformComponent>().value();
            worldRotation = parentTransform.transform.rotation * worldRotation;
            currentEntity = currentEntity.getParent();
        }

        return glm::normalize(worldRotation);
    }

    void lookAt(const ECSEntity& entity,
                glm::vec3 target,
                glm::vec3 upDirection = glm::vec3(0.0f, 1.0f, 0.0f)) {
        glm::vec3 worldPos = getWorldPosition(entity);
        glm::vec3 direction = target - worldPos;

        if (glm::length(direction) < 1e-8f) {
            return;
        }

        glm::vec3 f = glm::normalize(direction);
        glm::vec3 upNorm = glm::normalize(upDirection);

        if (std::abs(glm::dot(f, upNorm)) > 0.999f) {
            upNorm = (std::abs(f.y) < 0.999f) ? glm::vec3(0.0f, 1.0f, 0.0f)
                                              : glm::vec3(1.0f, 0.0f, 0.0f);
        }

        glm::vec3 r = glm::normalize(glm::cross(f, upNorm));
        glm::vec3 u = glm::normalize(glm::cross(r, f));
        glm::quat desiredWorldRotation = glm::quat_cast(glm::mat3(r, u, -f));

        ECSEntity parent = entity.getParent();
        if (parent.isValid() && parent.hasComponent<TransformComponent>()) {
            const TransformComponent& parentTransform =
                parent.getComponent<TransformComponent>().value();
            glm::quat parentWorldRotation = parentTransform.getWorldRotation(parent);
            transform.rotation =
                glm::normalize(glm::inverse(parentWorldRotation) * desiredWorldRotation);
        } else {
            transform.rotation = glm::normalize(desiredWorldRotation);
        }
    }

    bool operator==(const TransformComponent& other) const { return transform == other.transform; }
    bool operator!=(const TransformComponent& other) const { return !(*this == other); }

    Transform& operator*() { return transform; }
    const Transform& operator*() const { return transform; }
    Transform* operator->() { return &transform; }
    const Transform* operator->() const { return &transform; }
};

}  // namespace okay

#endif  // __TRANSFORM_COMPONENT_H__