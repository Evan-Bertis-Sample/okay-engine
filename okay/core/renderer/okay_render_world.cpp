#include "okay_render_world.hpp"
#include <queue>
#include "glm/ext/matrix_transform.hpp"

using namespace okay;

// OkayRenderWorld::ChildIterator

OkayRenderWorld::ChildIterator& OkayRenderWorld::ChildIterator::operator++() {
    if (_renderItem == RenderItemHandle::invalidHandle())
        return *this;
    const OkayRenderItem& item = world._renderItemPool.get(_renderItem);
    _renderItem = item.nextSibling;
    return *this;
}

bool OkayRenderWorld::ChildIterator::operator==(const ChildIterator& other) const {
    return _renderItem == other._renderItem;
}

bool OkayRenderWorld::ChildIterator::operator!=(const ChildIterator& other) const {
    return !(*this == other);
}

OkayRenderEntity OkayRenderWorld::ChildIterator::operator*() const {
    return world.getRenderEntity(_renderItem);
}

OkayRenderEntity OkayRenderWorld::ChildIterator::operator->() const {
    return world.getRenderEntity(_renderItem);
}

// OkayRenderEntity

OkayRenderEntity::Properties::~Properties() {
    // submit to render world for update
    _owner.updateEntity(_renderItem, std::move(*this));
}

OkayRenderEntity::Properties OkayRenderEntity::operator*() const {
    Properties p(_owner, _renderItem);
    const OkayRenderItem& item = _owner.getRenderItem(_renderItem);
    p.material = item.material;
    p.mesh = item.mesh;
    p.transform = item.transform;
    return p;
}

OkayRenderEntity::Properties OkayRenderEntity::operator->() const {
    return **this;
}

// OkayRenderWorld

OkayRenderWorld::ChildRange OkayRenderWorld::children(OkayRenderEntity parent) {
    if (!_renderItemPool.valid(parent._renderItem)) {
        return ChildRange(ChildIterator(*this, RenderItemHandle::invalidHandle()),
                          ChildIterator(*this, RenderItemHandle::invalidHandle()));
    }
    const OkayRenderItem& p = _renderItemPool.get(parent._renderItem);
    return ChildRange(ChildIterator(*this, p.firstChild),
                      ChildIterator(*this, RenderItemHandle::invalidHandle()));
}

const OkayRenderWorld::ChildRange OkayRenderWorld::children(OkayRenderEntity parent) const {
    return const_cast<OkayRenderWorld*>(this)->children(parent);
}

void OkayRenderWorld::rebuildTransforms() {
    // iterate through the dirty entities, and update their transforms

    // first, filter through the transforms
    // if any transform is in the same subtree as another dirty transform,
    // take the one higher in the tree
    std::queue<RenderItemHandle> workList;

    for (RenderItemHandle current : _dirtyTransforms) {
        RenderItemHandle parent = _renderItemPool.get(current).parent;
        bool foundParent = false;

        while (parent != RenderItemHandle::invalidHandle()) {
            if (_dirtyTransforms.find(parent) != _dirtyTransforms.end()) {
                current = parent;
                parent = _renderItemPool.get(current).parent;
            } else {
                foundParent = true;
                break;
            }
        }

        if (!foundParent) {
            // this is a top-level transform
            // update it's worldMatrix and add it to the worklist
            OkayRenderItem& item = _renderItemPool.get(current);
            glm::mat4 parentMat = item.parent == RenderItemHandle::invalidHandle()
                                      ? glm::identity<glm::mat4>()
                                      : _renderItemPool.get(item.parent).worldMatrix;

            item.worldMatrix = parentMat * item.transform.toMatrix();

            workList.push(current);
        }
    }

    // now recalculate the model matrix for each transform
    while (!workList.empty()) {
        RenderItemHandle root = workList.front();
        workList.pop();

        OkayRenderItem& rootItem = _renderItemPool.get(root);
        glm::mat4 rootMat = rootItem.transform.toMatrix();

        // update the children
        RenderItemHandle current = rootItem.firstChild;
        while (current != RenderItemHandle::invalidHandle()) {
            OkayRenderItem& child = _renderItemPool.get(current);
            glm::mat4 childMat = child.transform.toMatrix();
            child.worldMatrix = rootMat * childMat;
            workList.push(current);
            current = child.nextSibling;
        }
    }
}

void OkayRenderWorld::rebuildMaterials() {
    // resort the _memoizedRenderItems based on the sort key of the render item
    std::sort(
        _memoizedRenderItems.begin(),
        _memoizedRenderItems.end(),
        [this](const RenderItemHandle& a, const RenderItemHandle& b) { 
            return getRenderItem(a).sortKey < getRenderItem(b).sortKey;
        }
    );
}

void OkayRenderWorld::handleDirtyMesh(RenderItemHandle dirtyEntity) {
    // meshses don't affect anything
}

void OkayRenderWorld::handleDirtyMaterial(RenderItemHandle dirtyEntity) {
    _needsMaterialRebuild = true;
}

void OkayRenderWorld::handleDirtyTransform(RenderItemHandle dirtyEntity) {
    // is the entity already in the dirty set?
    if (_dirtyTransforms.find(dirtyEntity) != _dirtyTransforms.end()) {
        return;
    }

    _dirtyTransforms.insert(dirtyEntity);
}

const std::vector<RenderItemHandle>& OkayRenderWorld::getRenderItems() {
    if (_needsMaterialRebuild)
        rebuildMaterials();
    if (!_dirtyTransforms.empty())
        rebuildTransforms();
    return _memoizedRenderItems;
}

OkayRenderEntity OkayRenderWorld::addRenderEntity(const OkayTransform& transform,
                                                  OkayMaterial* material,
                                                  const OkayMesh& mesh,
                                                  OkayRenderEntity parent) {
    RenderItemHandle handle = _renderItemPool.emplace(material, mesh);
    // add this handle to the _memozedRenderItems vector
    _memoizedRenderItems.push_back(handle);
    OkayRenderItem& item = _renderItemPool.get(handle);

    item.transform = transform;
    item.material = material;
    item.mesh = mesh;

    OkayRenderEntity entity(*this, handle);

    // set up parent-child relationship
    if (parent._renderItem != RenderItemHandle::invalidHandle()) {
        Failable f = addChild(parent, entity);
        if (f.isError()) {
            // failed to add child, clean up and return invalid entity
            _renderItemPool.destroy(handle);
            return OkayRenderEntity(*this, RenderItemHandle::invalidHandle());
        }
    }

    // add to dirty set
    handleDirtyTransform(handle);
    handleDirtyMaterial(handle);
    handleDirtyMesh(handle);

    return entity;
}

Failable OkayRenderWorld::addChild(OkayRenderEntity parent, OkayRenderEntity children) {
    if (!_renderItemPool.valid(parent._renderItem)) {
        return Failable::errorResult("Parent render entity is invalid");
    }
    if (!_renderItemPool.valid(children._renderItem)) {
        return Failable::errorResult("Child render entity is invalid");
    }

    OkayRenderItem& parentItem = _renderItemPool.get(parent._renderItem);
    OkayRenderItem& childItem = _renderItemPool.get(children._renderItem);

    // now we need to check that the child item is not the parent of the parent item
    // to protect the invariant that there are no cycles in the scene graph
    if (isChildOf(children, parent)) {
        return Failable::errorResult("Cannot add child: would create cycle in scene graph");
    }

    // link the child to the parent
    childItem.parent = parent._renderItem;

    // insert child at the start of the parent's child list
    if (parentItem.firstChild == RenderItemHandle::invalidHandle()) {
        // parent has no children yet
        parentItem.firstChild = children._renderItem;
    } else {
        // parent has existing children, insert at start
        RenderItemHandle oldFirstChild = parentItem.firstChild;
        parentItem.firstChild = children._renderItem;
        childItem.nextSibling = oldFirstChild;
    }

    // mark the transforms dirty
    handleDirtyTransform(parent._renderItem);

    return Failable::ok({});
}

bool OkayRenderWorld::isChildOf(OkayRenderEntity parent, OkayRenderEntity child) const {
    if (!_renderItemPool.valid(parent._renderItem)) {
        return false;
    }
    if (!_renderItemPool.valid(child._renderItem)) {
        return false;
    }

    for (const OkayRenderEntity c : children(parent)) {
        if (c == child) {
            return true;
        }
        if (isChildOf(c, child)) {
            return true;
        }
    }

    return false;
}

void OkayRenderWorld::updateEntity(RenderItemHandle renderItem,
                                   const OkayRenderEntity::Properties&& properties) {
    if (!_renderItemPool.valid(renderItem)) {
        return;
    }

    OkayRenderItem& item = _renderItemPool.get(renderItem);

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
        handleDirtyTransform(renderItem);
    }
}

// OkayRenderItem

OkayRenderItem::OkayRenderItem(OkayMaterial* mat, OkayMesh m) : material(mat), mesh(m) {
    if (!mat || mesh.isEmpty()) {
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
