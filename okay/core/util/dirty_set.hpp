#ifndef __DIRTY_SET_H__
#define __DIRTY_SET_H__

#include <algorithm>
#include <cstdint>
#include <span>
#include <vector>

namespace okay {

template <typename IndexT = std::uint32_t>
class DirtySet final {
   public:
    DirtySet() = default;

    explicit DirtySet(std::size_t capacity) { _present.resize(capacity, 0); }

    void reserveItems(std::size_t n) { _items.reserve(n); }

    void ensureCapacity(std::size_t capacity) {
        if (_present.size() < capacity) _present.resize(capacity, 0);
    }

    bool contains(IndexT idx) const {
        const std::size_t i = static_cast<std::size_t>(idx);
        if (i >= _present.size()) return false;
        return _present[i] != 0;
    }

    bool insert(IndexT idx) {
        const std::size_t i = static_cast<std::size_t>(idx);
        if (i >= _present.size()) _present.resize(i + 1, 0);

        if (_present[i]) return false;

        _present[i] = 1;
        _items.push_back(idx);
        return true;
    }

    void erase(IndexT idx) {
        const std::size_t i = static_cast<std::size_t>(idx);
        if (i >= _present.size()) return;
        if (!_present[i]) return;

        _present[i] = 0;

        // Keep it simple: linear remove; usually you won't need erase().
        // If you do, we can add an index->position map.
        auto it = std::find(_items.begin(), _items.end(), idx);
        if (it != _items.end()) _items.erase(it);
    }

    void clear() {
        for (IndexT idx : _items) {
            const std::size_t i = static_cast<std::size_t>(idx);
            if (i < _present.size()) _present[i] = 0;
        }
        _items.clear();
    }

    bool empty() const { return _items.empty(); }
    std::size_t size() const { return _items.size(); }

    std::span<const IndexT> items() const {
        return std::span<const IndexT>(_items.data(), _items.size());
    }

    const std::vector<IndexT>& vec() const { return _items; }

   private:
    std::vector<IndexT> _items;
    std::vector<std::uint8_t> _present;
};

}  // namespace okay

#endif  // __DIRTY_SET_H__
