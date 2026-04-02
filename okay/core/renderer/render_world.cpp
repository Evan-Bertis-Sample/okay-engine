#include "render_world.hpp"

#include "glm/ext/matrix_transform.hpp"

#include <queue>

using namespace okay;

// OkayRenderWorld::ChildIterator

RenderWorld::ChildIterator& RenderWorld::ChildIterator::operator++() {
    if (_renderItem == RenderItemHandle::invalidHandle())
        return *this;
    const RenderItem& item = world._renderItemPool.get(_renderItem);
    _renderItem = item.nextSibling;
    return *this;
}

bool RenderWorld::ChildIterator::operator==(const ChildIterator& other) const {
    return _renderItem == other._renderItem;
}

bool RenderWorld::ChildIterator::operator!=(const ChildIterator& other) const {
    return !(*this == other);
}

RenderEntity RenderWorld::ChildIterator::operator*() const {
    return world.getRenderEntity(_renderItem);
}

RenderEntity RenderWorld::ChildIterator::operator->() const {
    return world.getRenderEntity(_renderItem);
}

// OkayRenderEntity

RenderEntity::Properties::~Properties() {
    // submit to render world for update
    _owner->updateEntity(_renderItem, std::move(*this));
}

RenderEntity::Properties RenderEntity::operator*() const {
    Properties p(_owner, _renderItem);
    const RenderItem& item = _owner->getRenderItem(_renderItem);
    p.material = item.material;
    p.mesh = item.mesh;
    p.transform = item.transform;
    return p;
}

RenderEntity::Properties RenderEntity::operator->() const {
    return **this;
}

bool RenderEntity::isValid() const {
    return _renderItem != RenderItemHandle::invalidHandle() && _owner != nullptr &&
           _owner->isValidEntity(*this);
}

// OkayRenderWorld

RenderWorld::ChildRange RenderWorld::children(RenderEntity parent) {
    if (!_renderItemPool.valid(parent._renderItem)) {
        return ChildRange(ChildIterator(*this, RenderItemHandle::invalidHandle()),
                          ChildIterator(*this, RenderItemHandle::invalidHandle()));
    }
    const RenderItem& p = _renderItemPool.get(parent._renderItem);
    return ChildRange(ChildIterator(*this, p.firstChild),
                      ChildIterator(*this, RenderItemHandle::invalidHandle()));
}

const RenderWorld::ChildRange RenderWorld::children(RenderEntity parent) const {
    return const_cast<RenderWorld*>(this)->children(parent);
}

void RenderWorld::rebuildTransforms() {
    if (_dirtyTransforms.empty())
        return;

    std::queue<RenderItemHandle> q;
    std::set<RenderItemHandle> dirtyRoots;
    // Find highest dirty ancestor for each dirty node, de-dup
    for (RenderItemHandle h : _dirtyTransforms) {
        RenderItemHandle r = h;
        while (true) {
            RenderItemHandle p = _renderItemPool.get(r).parent;
            if (p == RenderItemHandle::invalidHandle())
                break;
            if (_dirtyTransforms.find(p) == _dirtyTransforms.end())
                break;
            r = p;
        }
        dirtyRoots.insert(r);
    }

    // Seed queue with dirty roots, recompute their world from parent
    for (RenderItemHandle r : dirtyRoots) {
        RenderItem& item = _renderItemPool.get(r);
        glm::mat4 parentWorld = (item.parent == RenderItemHandle::invalidHandle())
                                    ? glm::identity<glm::mat4>()
                                    : _renderItemPool.get(item.parent).worldMatrix;

        item.worldMatrix = parentWorld * item.transform.toMatrix();
        q.push(r);
    }

    // BFS down each root, recompute children from parent.worldMatrix
    while (!q.empty()) {
        RenderItemHandle pHandle = q.front();
        q.pop();

        const RenderItem& parent = _renderItemPool.get(pHandle);
        RenderItemHandle c = parent.firstChild;

        while (c != RenderItemHandle::invalidHandle()) {
            RenderItem& child = _renderItemPool.get(c);
            child.worldMatrix = parent.worldMatrix * child.transform.toMatrix();
            q.push(c);
            c = child.nextSibling;
        }
    }

    _dirtyTransforms.clear();
}

void RenderWorld::rebuildMaterials() {
    // resort the _memoizedRenderItems based on the sort key of the render item
    std::sort(_memoizedRenderItems.begin(),
              _memoizedRenderItems.end(),
              [this](const RenderItemHandle& a, const RenderItemHandle& b) {
                  bool aValid = isValidEntity(a);
                  bool bValid = isValidEntity(b);

                  // Invalid always goes after valid
                  if (aValid != bValid)
                      return aValid;  // true < false → valid first

                  // Both invalid → keep order (return false)
                  if (!aValid)
                      return false;

                  // Both valid → compare normally
                  return getRenderItem(a).sortKey < getRenderItem(b).sortKey;
              });
}

void RenderWorld::handleDirtyMesh(RenderItemHandle dirtyEntity) {
    // meshses don't affect anything
}

void RenderWorld::handleDirtyMaterial(RenderItemHandle dirtyEntity) {
    _needsMaterialRebuild = true;
}

void RenderWorld::handleDirtyTransform(RenderItemHandle dirtyEntity) {
    // is the entity already in the dirty set?
    if (_dirtyTransforms.find(dirtyEntity) != _dirtyTransforms.end()) {
        return;
    }
    // Engine.logger.debug("Adding dirty transform {}", dirtyEntity.index);
    _dirtyTransforms.insert(dirtyEntity);
}

const std::span<RenderItemHandle> RenderWorld::getRenderItems() {
    if (_needsMaterialRebuild)
        rebuildMaterials();
    if (!_dirtyTransforms.empty())
        rebuildTransforms();
    return std::span<RenderItemHandle>(_memoizedRenderItems.data(), _activeRenderItems);
}

RenderEntity RenderWorld::addRenderEntity(const Transform& transform,
                                          const MaterialHandle& material,
                                          const Mesh& mesh,
                                          RenderEntity parent) {
    RenderItemHandle handle = _renderItemPool.emplace(material, mesh);
    // add this handle to the _memozedRenderItems vector
    _activeRenderItems++;
    if (_activeRenderItems > _memoizedRenderItems.size()) {
        _memoizedRenderItems.push_back(handle);
    } else {
        _memoizedRenderItems[_activeRenderItems - 1] = handle;
    }

    RenderItem& item = _renderItemPool.get(handle);

    item.transform = transform;
    item.material = material;
    item.mesh = mesh;

    RenderEntity entity(this, handle);

    // set up parent-child relationship
    if (parent._renderItem != RenderItemHandle::invalidHandle()) {
        Failable f = addChild(parent, entity);
        if (f.isError()) {
            // failed to add child, clean up and return invalid entity
            _renderItemPool.destroy(handle);
            return RenderEntity(this, RenderItemHandle::invalidHandle());
        }
    }

    // add to dirty set
    handleDirtyTransform(handle);
    handleDirtyMaterial(handle);
    handleDirtyMesh(handle);

    return entity;
}

void RenderWorld::removeRenderEntity(RenderItemHandle handle) {
    if (!_renderItemPool.valid(handle))
        return;

    // Remove children first
    while (true) {
        RenderItemHandle child = _renderItemPool.get(handle).firstChild;
        if (child == RenderItemHandle::invalidHandle())
            break;
        removeRenderEntity(child);
    }

    RenderItem& item = _renderItemPool.get(handle);
    RenderItemHandle parent = item.parent;

    // Unlink from parent sibling chain
    if (parent != RenderItemHandle::invalidHandle()) {
        RenderItem& parentItem = _renderItemPool.get(parent);
        if (parentItem.firstChild == handle) {
            parentItem.firstChild = item.nextSibling;
        } else {
            RenderItemHandle cur = parentItem.firstChild;
            while (cur != RenderItemHandle::invalidHandle()) {
                RenderItem& curItem = _renderItemPool.get(cur);
                if (curItem.nextSibling == handle) {
                    curItem.nextSibling = item.nextSibling;
                    break;
                }
                cur = curItem.nextSibling;
            }
        }
        handleDirtyTransform(parent);
    }

    _dirtyTransforms.erase(handle);

    // Remove from dense active list by value, not handle.index
    for (std::size_t i = 0; i < _activeRenderItems; ++i) {
        if (_memoizedRenderItems[i] == handle) {
            _memoizedRenderItems[i] = _memoizedRenderItems[_activeRenderItems - 1];
            --_activeRenderItems;
            break;
        }
    }

    _renderItemPool.destroy(handle);
}

Failable RenderWorld::addChild(RenderEntity parent, RenderEntity children) {
    if (!_renderItemPool.valid(parent._renderItem)) {
        return Failable::errorResult("Parent render entity is invalid");
    }
    if (!_renderItemPool.valid(children._renderItem)) {
        return Failable::errorResult("Child render entity is invalid");
    }

    RenderItem& parentItem = _renderItemPool.get(parent._renderItem);
    RenderItem& childItem = _renderItemPool.get(children._renderItem);

    // now we need to check that the child item is not the parent of the parent item
    // to protect the invariant that there are no cycles in the scene graph
    if (isChildOf(children, parent)) {
        return Failable::errorResult("Cannot add child: would create cycle in scene graph");
    }

    // link the child to the parent
    childItem.parent = parent._renderItem;

    childItem.nextSibling = parentItem.firstChild;  // invalid if none
    parentItem.firstChild = children._renderItem;

    // mark the transforms dirty
    handleDirtyTransform(parent._renderItem);

    return Failable::ok({});
}

bool RenderWorld::isChildOf(RenderEntity parent, RenderEntity child) const {
    if (!_renderItemPool.valid(parent._renderItem)) {
        return false;
    }
    if (!_renderItemPool.valid(child._renderItem)) {
        return false;
    }

    for (const RenderEntity c : children(parent)) {
        if (c == child) {
            return true;
        }
        if (isChildOf(c, child)) {
            return true;
        }
    }

    return false;
}

void RenderWorld::updateEntity(RenderItemHandle renderItem,
                               const RenderEntity::Properties&& properties) {
    if (!_renderItemPool.valid(renderItem)) {
        return;
    }

    RenderItem& item = _renderItemPool.get(renderItem);

    // check for changes and mark dirty as needed
    if (item.material != properties.material) {
        item.material = properties.material;
        handleDirtyMaterial(renderItem);
    }
    if (item.mesh != properties.mesh) {
        item.mesh = properties.mesh;
        handleDirtyMesh(renderItem);
    }
    if (item.transform != properties.transform) {
        item.transform = properties.transform;
    }
    handleDirtyTransform(renderItem);
}

// OkayRenderItem

RenderItem::RenderItem(MaterialHandle mat, Mesh m) : material(mat), mesh(m) {
    if (mat->isNone() || mesh.isEmpty()) {
        sortKey = std::numeric_limits<std::uint64_t>::max();
    } else {
        // 64 bit sort key
        // highest bits are shader id
        // lower bits are material id
        // this has the effect such that when you sort by sort key, materials are sorted into shader
        // buckets
        sortKey = (static_cast<std::uint64_t>(mat->shaderID()) << 32) |
                  static_cast<std::uint64_t>(mat->id());
    }
}
