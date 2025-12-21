#ifndef __OBJECT_POOL_H__
#define __OBJECT_POOL_H__

#include <cassert>
#include <cstdint>
#include <new>
#include <type_traits>
#include <utility>
#include <vector>

namespace okay {

template <typename T>
class ObjectPool final {
   public:
    struct Handle {
        std::uint32_t index{invalidIndex()};
        std::uint32_t generation{0};

        friend bool operator==(Handle a, Handle b) {
            return a.index == b.index && a.generation == b.generation;
        }
        friend bool operator!=(Handle a, Handle b) { return !(a == b); }
    };

    static constexpr std::uint32_t invalidIndex() { return 0xFFFFFFFFu; }

    ObjectPool() = default;
    ~ObjectPool() { clear(); }

    ObjectPool(const ObjectPool&) = delete;
    ObjectPool& operator=(const ObjectPool&) = delete;

    ObjectPool(ObjectPool&& other) noexcept { *this = std::move(other); }
    ObjectPool& operator=(ObjectPool&& other) noexcept {
        if (this == &other) return *this;

        clear();

        _slots = std::move(other._slots);
        _freeHead = other._freeHead;
        _aliveCount = other._aliveCount;

        other._freeHead = invalidIndex();
        other._aliveCount = 0;

        return *this;
    }

    template <typename... Args>
    Handle emplace(Args&&... args) {
        const std::uint32_t idx = allocateSlot();
        Slot& s = _slots[idx];

        ::new (s.ptr()) T(std::forward<Args>(args)...);

        s.alive = true;
        ++_aliveCount;

        return Handle{idx, s.generation};
    }

    void destroy(Handle h) {
        if (!valid(h)) return;

        Slot& s = _slots[h.index];
        assert(s.alive);

        s.ref().~T();
        s.alive = false;

        s.genera tion += 1;
        s.freeNext = _freeHead;
        _freeHead = h.index;

        assert(_aliveCount > 0);
        --_aliveCount;
    }

    void clear() {
        // destroy all live objects
        for (std::uint32_t i = 0; i < _slots.size(); ++i) {
            Slot& s = _slots[i];
            if (!s.alive) continue;
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

    bool valid(Handle h) const {
        if (h.index == invalidIndex()) return false;
        if (h.index >= _slots.size()) return false;
        const Slot& s = _slots[h.index];
        return s.alive && s.generation == h.generation;
    }

    std::size_t size() const { return _aliveCount; }
    std::size_t capacity() const { return _slots.size(); }

    T& get(Handle h) {
        assert(valid(h));
        return _slots[h.index].ref();
    }

    const T& get(Handle h) const {
        assert(valid(h));
        return _slots[h.index].ref();
    }

    T* tryGet(Handle h) {
        if (!valid(h)) return nullptr;
        return &_slots[h.index].ref();
    }

    const T* tryGet(Handle h) const {
        if (!valid(h)) return nullptr;
        return &_slots[h.index].ref();
    }

    bool aliveAt(std::uint32_t index) const {
        if (index >= _slots.size()) return false;
        return _slots[index].alive;
    }

    T& atIndex(std::uint32_t index) {
        assert(index < _slots.size());
        assert(_slots[index].alive);
        return _slots[index].ref();
    }

    const T& atIndex(std::uint32_t index) const {
        assert(index < _slots.size());
        assert(_slots[index].alive);
        return _slots[index].ref();
    }

    std::uint32_t generationAt(std::uint32_t index) const {
        assert(index < _slots.size());
        return _slots[index].generation;
    }

   private:
    struct Slot {
        std::uint32_t generation{1};
        bool alive{false};
        std::uint32_t freeNext{invalidIndex()};

        using Storage = std::aligned_storage_t<sizeof(T), alignof(T)>;
        Storage storage;

        T* ptr() { return reinterpret_cast<T*>(&storage); }
        const T* ptr() const { return reinterpret_cast<const T*>(&storage); }

        T& ref() { return *ptr(); }
        const T& ref() const { return *ptr(); }
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
