#ifndef __OBJECT_POOL_H__
#define __OBJECT_POOL_H__

#include <okay/core/engine/engine.hpp>
#include <okay/core/engine/logger.hpp>

#include <cstdint>
#include <iterator>
#include <type_traits>
#include <utility>
#include <vector>

namespace okay {

struct ObjectPoolHandle {
    std::uint32_t index{invalidIndex()};
    std::uint32_t generation{0};

    ObjectPoolHandle() = default;
    ObjectPoolHandle(std::uint32_t index) : index(index) {}
    ObjectPoolHandle(std::uint32_t index, std::uint32_t generation)
        : index(index), generation(generation) {}

    friend bool operator==(ObjectPoolHandle a, ObjectPoolHandle b) {
        return a.index == b.index && a.generation == b.generation;
    }
    friend bool operator!=(ObjectPoolHandle a, ObjectPoolHandle b) {
        return !(a == b);
    }

    static constexpr std::uint32_t invalidIndex() {
        return 0xFFFFFFFFu;
    }
    static ObjectPoolHandle invalidHandle() {
        return ObjectPoolHandle(invalidIndex());
    }

    // operator overloads for std::map
    bool operator<(const ObjectPoolHandle& other) const {
        return index < other.index;
    }
    bool operator>(const ObjectPoolHandle& other) const {
        return index > other.index;
    }
    bool operator<=(const ObjectPoolHandle& other) const {
        return index <= other.index;
    }
    bool operator>=(const ObjectPoolHandle& other) const {
        return index >= other.index;
    }
};

template <typename T>
class ObjectPool final {
   public:
    template <bool IsConst>
    class IteratorBase {
       private:
        using PoolType = std::conditional_t<IsConst, const ObjectPool<T>, ObjectPool<T>>;

       public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = std::conditional_t<IsConst, const T*, T*>;
        using reference = std::conditional_t<IsConst, const T&, T&>;

        IteratorBase() = default;

        IteratorBase(PoolType* pool, std::uint32_t index) : _pool(pool), _index(index) {
            skipDead();
        }

        reference operator*() const {
            return _pool->_slots[_index].ref();
        }

        pointer operator->() const {
            return &_pool->_slots[_index].ref();
        }

        IteratorBase& operator++() {
            ++_index;
            skipDead();
            return *this;
        }

        IteratorBase operator++(int) {
            IteratorBase temp = *this;
            ++(*this);
            return temp;
        }

        bool operator==(const IteratorBase& other) const {
            return _pool == other._pool && _index == other._index;
        }

        bool operator!=(const IteratorBase& other) const {
            return !(*this == other);
        }

       private:
        void skipDead() {
            while (
                _pool != nullptr && _index < _pool->_slots.size() && !_pool->_slots[_index].alive) {
                ++_index;
            }
        }

        PoolType* _pool{nullptr};
        std::uint32_t _index{0};
    };

    using Iterator = IteratorBase<false>;
    using ConstIterator = IteratorBase<true>;

    static constexpr std::uint32_t invalidIndex() {
        return ObjectPoolHandle::invalidIndex();
    }
    static ObjectPoolHandle invalidHandle() {
        return ObjectPoolHandle::invalidHandle();
    }

    ObjectPool() = default;
    ~ObjectPool() {
        clear();
    }

    ObjectPool(const ObjectPool&) = delete;
    ObjectPool& operator=(const ObjectPool&) = delete;

    ObjectPool(ObjectPool&& other) noexcept {
        *this = std::move(other);
    }
    ObjectPool& operator=(ObjectPool&& other) noexcept {
        if (this == &other)
            return *this;

        clear();

        _slots = std::move(other._slots);
        _freeHead = other._freeHead;
        _aliveCount = other._aliveCount;

        other._freeHead = invalidIndex();
        other._aliveCount = 0;

        return *this;
    }

    template <typename... Args>
    ObjectPoolHandle emplace(Args&&... args) {
        const std::uint32_t idx = allocateSlot();
        Slot& s = _slots[idx];

        ::new (s.ptr()) T(std::forward<Args>(args)...);

        s.alive = true;
        ++_aliveCount;

        return ObjectPoolHandle{idx, s.generation};
    }

    void destroy(ObjectPoolHandle h) {
        if (!valid(h))
            return;

        Slot& s = _slots[h.index];
        if (!s.alive) {
            Engine.logger.error("invalid handle");
            return;
        }

        s.ref().~T();
        s.alive = false;

        s.generation += 1;
        s.freeNext = _freeHead;
        _freeHead = h.index;

        if (_aliveCount <= 0) {
            Engine.logger.error("aliveCount <= 0");
            return;
        }

        --_aliveCount;
    }

    void clear() {
        // destroy all live objects
        for (std::uint32_t i = 0; i < _slots.size(); ++i) {
            Slot& s = _slots[i];
            if (!s.alive)
                continue;
            s.ref().~T();
            s.alive = false;
            s.generation += 1;
        }

        // rebuild free list
        _freeHead = invalidIndex();
        for (std::uint32_t i = static_cast<std::uint32_t>(_slots.size()); i-- > 0;) {
            _slots[i].freeNext = _freeHead;
            _freeHead = i;
        }

        _aliveCount = 0;
    }

    bool valid(ObjectPoolHandle h) const {
        if (h.index == invalidIndex())
            return false;
        if (h.index >= _slots.size())
            return false;
        const Slot& s = _slots[h.index];
        return s.alive && s.generation == h.generation;
    }

    std::size_t size() const {
        return _aliveCount;
    }
    std::size_t capacity() const {
        return _slots.size();
    }

    T& get(ObjectPoolHandle h) {
        if (!valid(h)) {
            Engine.logger.error("invalid handle");
            return _slots[0].ref();
        }

        return _slots[h.index].ref();
    }

    const T& get(ObjectPoolHandle h) const {
        if (!valid(h)) {
            Engine.logger.error("invalid handle");
            return _slots[0].ref();
        }

        return _slots[h.index].ref();
    }

    T* tryGet(ObjectPoolHandle h) {
        if (!valid(h)) {
            Engine.logger.error("invalid handle");
            return nullptr;
        }

        return &_slots[h.index].ref();
    }

    const T* tryGet(ObjectPoolHandle h) const {
        if (!valid(h)) {
            Engine.logger.error("invalid handle");
            return nullptr;
        }

        return &_slots[h.index].ref();
    }

    bool aliveAt(std::uint32_t index) const {
        if (index >= _slots.size())
            return false;
        return _slots[index].alive;
    }

    T& atIndex(std::uint32_t index) {
        if (index >= _slots.size()) {
            Engine.logger.error("invalid index");
            return _slots[0].ref();
        }

        if (!aliveAt(index)) {
            Engine.logger.error("invalid index");
            return _slots[0].ref();
        }

        return _slots[index].ref();
    }

    const T& atIndex(std::uint32_t index) const {
        if (index >= _slots.size()) {
            Engine.logger.error("invalid index");
            return _slots[0].ref();
        }

        if (!aliveAt(index)) {
            Engine.logger.error("invalid index");
            return _slots[0].ref();
        }

        return _slots[index].ref();
    }

    std::uint32_t generationAt(std::uint32_t index) const {
        if (index >= _slots.size()) {
            Engine.logger.error("invalid index");
            return 0;
        }

        return _slots[index].generation;
    }

    Iterator begin() {
        return Iterator(this, 0);
    }

    Iterator end() {
        return Iterator(this, static_cast<std::uint32_t>(_slots.size()));
    }

    ConstIterator begin() const {
        return ConstIterator(this, 0);
    }

    ConstIterator end() const {
        return ConstIterator(this, static_cast<std::uint32_t>(_slots.size()));
    }

   private:
    struct Slot {
        std::uint32_t generation{1};
        bool alive{false};
        std::uint32_t freeNext{invalidIndex()};

        using Storage = std::aligned_storage_t<sizeof(T), alignof(T)>;
        Storage storage;

        T* ptr() {
            return reinterpret_cast<T*>(&storage);
        }
        const T* ptr() const {
            return reinterpret_cast<const T*>(&storage);
        }

        T& ref() {
            return *ptr();
        }
        const T& ref() const {
            return *ptr();
        }
    };

    std::vector<Slot> _slots;
    std::uint32_t _freeHead{invalidIndex()};
    std::size_t _aliveCount{0};

    std::uint32_t allocateSlot() {
        if (_freeHead != invalidIndex()) {
            const std::uint32_t idx = _freeHead;
            Slot& s = _slots[idx];
            _freeHead = s.freeNext;
            s.freeNext = invalidIndex();
            // s.alive remains false until emplace constructs
            return idx;
        }

        _slots.emplace_back();
        return static_cast<std::uint32_t>(_slots.size() - 1);
    }
};

}  // namespace okay

#endif  // __OBJECT_POOL_H__