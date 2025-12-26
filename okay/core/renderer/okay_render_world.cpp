#include "okay_render_world.hpp"

using namespace okay;

// OkayRenderWorld::ChildIterator

OkayRenderWorld::ChildIterator& OkayRenderWorld::ChildIterator::operator++() {
    if (!world || handle == ItemHandle::invalidHandle()) return *this;
    const OkayRenderItem& item = world->_renderItemPool.get(handle);
    handle = item.nextSibling;
    return *this;
}

bool OkayRenderWorld::ChildIterator::operator==(const ChildIterator& other) const {
    return handle == other.handle;
}

bool OkayRenderWorld::ChildIterator::operator!=(const ChildIterator& other) const {
    return !(*this == other);
}

const OkayRenderItem& OkayRenderWorld::ChildIterator::operator*() const {
    return world->_renderItemPool.get(handle);
}

const OkayRenderItem* OkayRenderWorld::ChildIterator::operator->() const {
    return &world->_renderItemPool.get(handle);
}

// OkayRenderWorld

OkayRenderWorld::ChildRange OkayRenderWorld::children(ItemHandle parent) const {
    if (!_renderItemPool.valid(parent)) {
        return ChildRange{ChildIterator(this, ItemHandle::invalidHandle()),
                          ChildIterator(this, ItemHandle::invalidHandle())};
    }
    const OkayRenderItem& p = _renderItemPool.get(parent);
    return ChildRange{ChildIterator(this, p.firstChild),
                      ChildIterator(this, ItemHandle::invalidHandle())};
}

void OkayRenderWorld::rebuildTransforms() {
    for (EntityHandle h : _dirtyEntities) {
        if (!_entityPool.valid(h)) continue;

        OkayRenderEntity& entity = _entityPool.get(h);
        if (!_renderItemPool.valid(entity.renderItem)) continue;

        OkayRenderItem& item = _renderItemPool.get(entity.renderItem);

        const OkayTransform& t = entity.transform.get();

        glm::mat4 translationMat = glm::translate(glm::mat4(1.0f), t.position);
        glm::mat4 rotationMat = glm::mat4_cast(t.rotation);
        glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f), t.scale);
  
        item.worldMatrix = translationMat * rotationMat * scaleMat;
    }

    _dirtyEntities.clear();
}

void OkayRenderWorld::rebuildMaterials() {
    _memoizedRenderItems.clear();

    for (std::uint32_t i = 0; i < static_cast<std::uint32_t>(_entityPool.capacity()); ++i) {
        if (!_entityPool.aliveAt(i)) continue;

        EntityHandle eh{i, _entityPool.generationAt(i)};
        OkayRenderEntity& entity = _entityPool.get(eh);

        if (_renderItemPool.valid(entity.renderItem)) {
            _renderItemPool.destroy(entity.renderItem);
            entity.renderItem = ItemHandle::invalidHandle();
        }

        const IOkayMaterial* mat = entity.material.get();
        OkayMesh mesh = entity.mesh.get();

        OkayRenderItem newItem(mat, mesh);
        ItemHandle ih = _renderItemPool.emplace(std::move(newItem));
        entity.renderItem = ih;

        _memoizedRenderItems.push_back(&_renderItemPool.get(ih));
    }

    std::sort(
        _memoizedRenderItems.begin(), _memoizedRenderItems.end(),
        [](const OkayRenderItem* a, const OkayRenderItem* b) { return a->sortKey < b->sortKey; });

    _needsMaterialRebuild = false;
}

void OkayRenderWorld::handleDirtyMesh(void* ctx) {
    auto* c = static_cast<DirtyContext*>(ctx);
    if (!c || !c->world) return;
    if (!c->world->_entityPool.valid(c->entity)) return;
}

void OkayRenderWorld::handleDirtyTransform(void* ctx) {
    auto* c = static_cast<DirtyContext*>(ctx);
    if (!c || !c->world) return;
    if (!c->world->_entityPool.valid(c->entity)) return;
}

void OkayRenderWorld::handleDirtyMaterial(void* ctx) {
    auto* c = static_cast<DirtyContext*>(ctx);
    if (!c || !c->world) return;
    c->world->_needsMaterialRebuild = true;
}

const std::vector<const OkayRenderItem*>& OkayRenderWorld::getRenderItems() {
    if (_needsMaterialRebuild) rebuildMaterials();
    if (!_dirtyEntities.empty()) rebuildTransforms();
    return _memoizedRenderItems;
}

EntityHandle OkayRenderWorld::addRenderEntity(const OkayTransform& transform,
                                              IOkayMaterial* material, const OkayMesh& mesh) {}

// OkayRenderItem

OkayRenderItem::OkayRenderItem(const IOkayMaterial* mat, OkayMesh m) : material(mat), mesh(m) {
    if (!mat || mesh.isEmpty()) {
        sortKey = std::numeric_limits<std::uint64_t>::max();
    } else {
        // 64 bit sort key
        // highest bits are shader id
        // lower bits are material id
        // this has the effect such that when you sort by sort key, materials are sorted into shader buckets
        sortKey = (static_cast<std::uint64_t>(mat->shaderID()) << 32) | static_cast<std::uint64_t>(mat->id());
    }
}
