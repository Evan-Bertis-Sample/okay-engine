#ifndef __TRANSFORM_COMPONENT_H__
#define __TRANSFORM_COMPONENT_H__

#include <okay/core/ecs/ecs.hpp>
#include <okay/core/renderer/render_world.hpp>

namespace okay {

struct TransformComponent {
    Transform transform;

    TransformComponent(const glm::vec3& position = glm::vec3(0.0f),
                       const glm::vec3& scale = glm::vec3(1.0f),
                       const glm::quat& rotation = glm::quat())
        : transform(position, scale, rotation) {}
    TransformComponent(const Transform& transform) : transform(transform) {}

    glm::mat4 toMatrix() const { return transform.toMatrix(); }
    glm::vec3 localForward() const { return transform.rotation * glm::vec3(0, 0, -1); }
    glm::vec3 localUp() const { return transform.rotation * glm::vec3(0, 1, 0); }
    glm::vec3 localRight() const { return transform.rotation * glm::vec3(1, 0, 0); }

    void setLocalDirection(glm::vec3 direction, glm::vec3 up = glm::vec3(0, 1, 0)) {
        // create a lookAt matrix and extract the rotation from it
        glm::mat4 lookAt = glm::lookAt(transform.position, transform.position + direction, up);
        glm::mat3 worldRot = glm::inverse(glm::mat3(lookAt));
        transform.rotation = glm::quat_cast(worldRot);
    }

    glm::mat4 getWorldMatrix(const ECSEntity& entity) const {
        glm::mat4 worldMatrix = transform.toMatrix();
        ECSEntity currentEntity = entity.getParent();
        while (currentEntity.isValid() && currentEntity.hasComponent<TransformComponent>()) {
            Engine.logger.debug("TransformComponent: Multiplying parent transform for entity {}",
                                currentEntity.id());
            const TransformComponent& parentTransform =
                currentEntity.getComponent<TransformComponent>().value();
            worldMatrix = parentTransform.transform.toMatrix() * worldMatrix;
            currentEntity = currentEntity.getParent();
        }

        return worldMatrix;
    };

    glm::vec3 forward(const ECSEntity& entity) const {
        return getWorldMatrix(entity) * glm::vec4(0, 0, -1, 0);
    }
    glm::vec3 up(const ECSEntity& entity) const {
        return getWorldMatrix(entity) * glm::vec4(0, 1, 0, 0);
    }
    glm::vec3 right(const ECSEntity& entity) const {
        return getWorldMatrix(entity) * glm::vec4(1, 0, 0, 0);
    }

    glm::vec3 getWorldPosition(const ECSEntity& entity) const {
        glm::mat4 worldMatrix = getWorldMatrix(entity);
        return glm::vec3(worldMatrix[3]);  // extract translation from world matrix
    }

    glm::vec3 getWorldScale(const ECSEntity& entity) const {
        glm::mat4 worldMatrix = getWorldMatrix(entity);
        return glm::vec3(worldMatrix[0][0],
                         worldMatrix[1][1],
                         worldMatrix[2][2]);  // extract scale from world matrix
    }

    glm::quat getWorldRotation(const ECSEntity& entity) const {
        glm::mat4 worldMatrix = getWorldMatrix(entity);
        glm::mat3 rotMatrix(worldMatrix);
        return glm::quat_cast(rotMatrix);  // extract rotation from world matrix
    }

    void lookAt(const ECSEntity& entity, glm::vec3 target, glm::vec3 up = glm::vec3(0, 1, 0)) {
        glm::vec3 worldPos = getWorldPosition(entity);
        glm::vec3 direction = target - worldPos;
        setLocalDirection(direction, up);
    }

    bool operator==(const TransformComponent& other) const { return transform == other.transform; }
    bool operator!=(const TransformComponent& other) const { return !(*this == other); }

    Transform& operator*() { return transform; }
    const Transform& operator*() const { return transform; }
    Transform* operator->() { return &transform; }
    const Transform* operator->() const { return &transform; }
};

};  // namespace okay

#endif  // __TRANSFORM_COMPONENT_H__