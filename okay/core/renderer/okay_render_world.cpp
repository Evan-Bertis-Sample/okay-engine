#include <okay/core/renderer/okay_render_world.hpp>
#include <algorithm>
#include <cassert>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

using namespace okay;

void SceneHierarchy::resize(std::size_t n) {
    _nodes.resize(n);
}

std::uint32_t SceneHierarchy::parentOf(std::uint32_t idx) const {
    return _nodes[idx].parent;
}
std::uint32_t SceneHierarchy::firstChildOf(std::uint32_t idx) const {
    return _nodes[idx].firstChild;
}
std::uint32_t SceneHierarchy::nextSiblingOf(std::uint32_t idx) const {
    return _nodes[idx].nextSibling;
}
std::uint32_t SceneHierarchy::prevSiblingOf(std::uint32_t idx) const {
    return _nodes[idx].prevSibling;
}

void SceneHierarchy::detach(std::uint32_t child) {
    if (child == NullIndex) return;

    Node& c = _nodes[child];
    const std::uint32_t pIdx = c.parent;
    if (pIdx == NullIndex) {
        c.prevSibling = NullIndex;
        c.nextSibling = NullIndex;
        return;
    }

    Node& p = _nodes[pIdx];

    const std::uint32_t prev = c.prevSibling;
    const std::uint32_t next = c.nextSibling;

    if (prev != NullIndex) {
        _nodes[prev].nextSibling = next;
    } else {
        p.firstChild = next;
    }

    if (next != NullIndex) {
        _nodes[next].prevSibling = prev;
    }

    c.parent = NullIndex;
    c.prevSibling = NullIndex;
    c.nextSibling = NullIndex;
}

void SceneHierarchy::attach(std::uint32_t child, std::uint32_t parent) {
    if (child == NullIndex) return;

    detach(child);

    if (parent == NullIndex) return;

    Node& c = _nodes[child];
    Node& p = _nodes[parent];

    c.parent = parent;
    c.prevSibling = NullIndex;
    c.nextSibling = p.firstChild;

    if (p.firstChild != NullIndex) {
        _nodes[p.firstChild].prevSibling = child;
    }

    p.firstChild = child;
}

ChildrenView SceneHierarchy::children(std::uint32_t parent) const {
    std::vector<std::uint32_t> out;
    if (parent == NullIndex) return ChildrenView{std::move(out)};

    std::uint32_t cur = _nodes[parent].firstChild;
    while (cur != NullIndex) {
        out.push_back(cur);
        cur = _nodes[cur].nextSibling;
    }

    return ChildrenView{std::move(out)};
}

void SceneHierarchy::onDestroy(std::uint32_t idx) {
    if (idx == NullIndex) return;

    detach(idx);

    std::uint32_t c = _nodes[idx].firstChild;
    while (c != NullIndex) {
        std::uint32_t next = _nodes[c].nextSibling;
        _nodes[c].parent = NullIndex;
        _nodes[c].prevSibling = NullIndex;
        _nodes[c].nextSibling = NullIndex;
        c = next;
    }

    _nodes[idx].firstChild = NullIndex;
    _nodes[idx].parent = NullIndex;
    _nodes[idx].prevSibling = NullIndex;
    _nodes[idx].nextSibling = NullIndex;
}

// ---------------- OkayRenderWorld ----------------

static inline bool isNullIndex(std::uint32_t i) {
    return i == OkayRenderWorld::NullIndex;
}

OkayRenderWorld::Slot& OkayRenderWorld::slotRef(Entity e) {
    return _pool.get(e);
}

const OkayRenderWorld::Slot& OkayRenderWorld::slotRef(Entity e) const {
    return _pool.get(e);
}

OkayTransform& OkayRenderWorld::transformRef(Entity e) {
    return slotRef(e).local;
}
OkayMesh& OkayRenderWorld::meshRef(Entity e) {
    return slotRef(e).mesh;
}
IOkayMaterial& OkayRenderWorld::materialRef(Entity e) {
    return slotRef(e).material;
}

void OkayRenderWorld::ensureIndexCapacity(std::uint32_t index) {
    const std::size_t need = static_cast<std::size_t>(index) + 1;
    if (_entityToDrawIndex.size() < need) _entityToDrawIndex.resize(need, NullIndex);
    _dirtyTransformRoots.ensureCapacity(need);
    _hierarchy.resize(need);
}

OkayRenderWorld::Entity OkayRenderWorld::spawn(const OkayTransform& localTransform,
                                               const OkayMesh& mesh, const IOkayMaterial& material,
                                               Entity parent) {
    Entity e = _pool.emplace();
    ensureIndexCapacity(e.index);

    Slot& s = _pool.get(e);
    s.local = localTransform;
    s.world = glm::mat4(1.0f);
    s.mesh = mesh;
    s.material = material;

    if (_pool.valid(parent)) {
        ensureIndexCapacity(parent.index);
        _hierarchy.attach(e.index, parent.index);
        markSubtreeTransformDirty(e.index);
    } else {
        markTransformDirty(e.index);
    }

    markRenderablesDirty();
    return e;
}

void OkayRenderWorld::destroy(Entity e) {
    if (!_pool.valid(e)) return;

    const std::uint32_t idx = e.index;

    if (idx < _entityToDrawIndex.size()) _entityToDrawIndex[idx] = NullIndex;

    _hierarchy.onDestroy(idx);
    _pool.destroy(e);

    markRenderablesDirty();
}

OkayRenderWorld::Entity OkayRenderWorld::parentOf(Entity e) const {
    assert(_pool.valid(e));
    const std::uint32_t p = _hierarchy.parentOf(e.index);
    if (p == SceneHierarchy::NullIndex) return NullEntity;
    return Entity{p, _pool.generationAt(p)};
}

ChildrenView OkayRenderWorld::children(Entity e) const {
    assert(_pool.valid(e));
    return _hierarchy.children(e.index);
}

void OkayRenderWorld::attach(Entity child, Entity newParent) {
    assert(_pool.valid(child));

    ensureIndexCapacity(child.index);

    if (_pool.valid(newParent)) {
        ensureIndexCapacity(newParent.index);
        _hierarchy.attach(child.index, newParent.index);
    } else {
        _hierarchy.detach(child.index);
    }

    markSubtreeTransformDirty(child.index);
}

void OkayRenderWorld::detach(Entity child) {
    assert(_pool.valid(child));
    _hierarchy.detach(child.index);
    markSubtreeTransformDirty(child.index);
}

const OkayTransform& OkayRenderWorld::localTransform(Entity e) const {
    assert(_pool.valid(e));
    return _pool.get(e).local;
}

const glm::mat4& OkayRenderWorld::worldFromEntity(Entity e) const {
    assert(_pool.valid(e));
    return _pool.get(e).world;
}

const OkayMesh& OkayRenderWorld::mesh(Entity e) const {
    assert(_pool.valid(e));
    return _pool.get(e).mesh;
}

const IOkayMaterial& OkayRenderWorld::material(Entity e) const {
    assert(_pool.valid(e));
    return _pool.get(e).material;
}

void OkayRenderWorld::setTransform(Entity e, const OkayTransform& t) {
    assert(_pool.valid(e));
    _pool.get(e).local = t;
    markTransformDirty(e.index);
}

void OkayRenderWorld::setMesh(Entity e, const OkayMesh& mesh) {
    assert(_pool.valid(e));
    _pool.get(e).mesh = mesh;
    markRenderablesDirty();
}

void OkayRenderWorld::setMaterial(Entity e, const IOkayMaterial& material) {
    assert(_pool.valid(e));
    _pool.get(e).material = material;
    markRenderablesDirty();
}

void OkayRenderWorld::markTransformDirty(std::uint32_t index) {
    if (isNullIndex(index)) return;
    if (!_pool.aliveAt(index)) return;

    std::uint32_t cur = index;
    while (true) {
        const std::uint32_t p = _hierarchy.parentOf(cur);
        if (p == SceneHierarchy::NullIndex) break;
        if (!_pool.aliveAt(p)) break;
        if (_dirtyTransformRoots.contains(p)) return;
        cur = p;
    }

    _dirtyTransformRoots.insert(cur);
}

void OkayRenderWorld::markSubtreeTransformDirty(std::uint32_t index) {
    markTransformDirty(index);
}

void OkayRenderWorld::processDirtyTransforms() {
    auto roots = _dirtyTransformRoots.items();
    for (std::uint32_t root : roots) {
        if (!_pool.aliveAt(root)) continue;

        const std::uint32_t p = _hierarchy.parentOf(root);
        glm::mat4 parentWorld{1.0f};

        if (p != SceneHierarchy::NullIndex && _pool.aliveAt(p)) {
            parentWorld = _pool.atIndex(p).world;
        }

        updateWorldSubtree(root, parentWorld);
    }

    _dirtyTransformRoots.clear();
}

void OkayRenderWorld::updateWorldSubtree(std::uint32_t rootIndex, const glm::mat4& parentWorld) {
    struct Item {
        std::uint32_t idx;
        glm::mat4 parent;
    };

    std::vector<Item> stack;
    stack.push_back(Item{rootIndex, parentWorld});

    while (!stack.empty()) {
        Item it = stack.back();
        stack.pop_back();

        if (!_pool.aliveAt(it.idx)) continue;

        Slot& s = _pool.atIndex(it.idx);
        s.world = it.parent * compose(s.local);

        syncDrawItemTransform(it.idx);

        const glm::mat4 w = s.world;

        std::uint32_t c = _hierarchy.firstChildOf(it.idx);
        while (c != SceneHierarchy::NullIndex) {
            stack.push_back(Item{c, w});
            c = _hierarchy.nextSiblingOf(c);
        }
    }
}

void OkayRenderWorld::rebuildDrawList() {
    _drawItems.clear();

    const std::size_t cap = _pool.capacity();
    _drawItems.reserve(_pool.size());

    if (_entityToDrawIndex.size() < cap) _entityToDrawIndex.resize(cap, NullIndex);
    std::fill(_entityToDrawIndex.begin(), _entityToDrawIndex.end(), NullIndex);

    for (std::uint32_t i = 0; i < cap; ++i) {
        if (!_pool.aliveAt(i)) continue;

        const Slot& s = _pool.atIndex(i);

        OkayDrawItem di;
        di.worldFromMesh = s.world;
        di.mesh = s.mesh;
        di.material = s.material;
        di.sortKey = makeSortKey(di.material, di.mesh);
        di.entityIndex = i;

        _entityToDrawIndex[i] = static_cast<std::uint32_t>(_drawItems.size());
        _drawItems.push_back(std::move(di));
    }

    std::sort(_drawItems.begin(), _drawItems.end(),
              [](const OkayDrawItem& a, const OkayDrawItem& b) { return a.sortKey < b.sortKey; });

    for (std::uint32_t i = 0; i < static_cast<std::uint32_t>(_drawItems.size()); ++i) {
        const std::uint32_t ent = _drawItems[i].entityIndex;
        if (ent < _entityToDrawIndex.size()) _entityToDrawIndex[ent] = i;
    }
}

void OkayRenderWorld::syncDrawItemTransform(std::uint32_t index) {
    if (index >= _entityToDrawIndex.size()) return;
    const std::uint32_t di = _entityToDrawIndex[index];
    if (di == NullIndex) return;
    if (di >= _drawItems.size()) return;
    _drawItems[di].worldFromMesh = _pool.atIndex(index).world;
}

std::span<const OkayDrawItem> OkayRenderWorld::drawItems() {
    processDirtyTransforms();

    if (_renderablesDirty) {
        rebuildDrawList();
        _renderablesDirty = false;
    }

    return std::span<const OkayDrawItem>(_drawItems.data(), _drawItems.size());
}

void OkayRenderWorld::invalidateAll() {
    _dirtyTransformRoots.clear();

    const std::size_t cap = _pool.capacity();
    _dirtyTransformRoots.ensureCapacity(cap);

    for (std::uint32_t i = 0; i < cap; ++i) {
        if (!_pool.aliveAt(i)) continue;
        if (_hierarchy.parentOf(i) == SceneHierarchy::NullIndex) {
            _dirtyTransformRoots.insert(i);
        }
    }

    _renderablesDirty = true;
}

glm::mat4 OkayRenderWorld::compose(const OkayTransform& t) {
    const glm::mat4 T = glm::translate(glm::mat4(1.0f), t.position);
    const glm::mat4 R = glm::toMat4(t.rotation);
    const glm::mat4 S = glm::scale(glm::mat4(1.0f), t.scale);
    return T * R * S;
}

std::uint64_t OkayRenderWorld::makeSortKey(const IOkayMaterial& material, const OkayMesh& mesh) {
    const std::uint64_t shader = static_cast<std::uint64_t>(material.shaderID());
    const std::uint64_t matId = static_cast<std::uint64_t>(material.id());

    std::uint64_t meshBits = static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(&mesh));
    meshBits ^= meshBits >> 33;
    meshBits *= 0xff51afd7ed558ccdULL;
    meshBits ^= meshBits >> 33;
    meshBits *= 0xc4ceb9fe1a85ec53ULL;
    meshBits ^= meshBits >> 33;

    return (shader << 32) ^ (matId << 16) ^ (meshBits & 0xFFFFULL);
}

