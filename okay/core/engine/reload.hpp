#ifndef __RELOAD_HPP__
#define __RELOAD_HPP__

#include "okay/core/util/option.hpp"

#include <cstdint>
#include <cstring>
#include <functional>
#include <iostream>
#include <map>
#include <span>
#include <string_view>
#include <type_traits>
#include <vector>

namespace okay {

struct ReloadContext {
   public:
    using ReloadID = std::size_t;

    struct WriteBlob {
        ReloadID id;
        std::vector<std::uint8_t> blob;

        void write(const std::uint8_t* data, std::size_t size) {
            const std::size_t start = blob.size();
            blob.resize(start + size);

            if (size > 0) {
                std::memcpy(blob.data() + start, data, size);
            }
        }

        template <typename T>
        void write(const T& t) {
            static_assert(std::is_trivially_copyable_v<T>);
            write(reinterpret_cast<const std::uint8_t*>(&t), sizeof(T));
        }

        ~WriteBlob() {
            _owner->submitWriteBlob(*this);
        }

       private:
        ReloadContext* _owner;

        WriteBlob(ReloadContext* owner, ReloadID id) : id(id), _owner(owner) {}

        friend struct ReloadContext;
    };

    struct ReadBlob {
        std::span<std::uint8_t> blob;

        template <typename T>
        T* read() {
            static_assert(std::is_trivially_copyable_v<T>);

            if (validRead(sizeof(T))) {
                T* valuePtr = reinterpret_cast<T*>(blob.data() + _current);

                progress(sizeof(T));
                return valuePtr;
            }

            std::cout << "Unable to read from reload context from " << _current << " to "
                      << _current + sizeof(T) << '\n';

            return nullptr;
        }

        bool progress(std::size_t n) {
            if (!validRead(n)) {
                return false;
            }

            _current += n;
            return true;
        }

        bool validRead(std::size_t n) const {
            return _current <= blob.size() && n <= blob.size() - _current;
        }

       private:
        std::size_t _current{0};

        explicit ReadBlob(std::span<std::uint8_t> blob) : blob(blob) {}

        friend struct ReloadContext;
    };

    WriteBlob createWriteBlob(std::string_view blobName) {
        return WriteBlob(this, getReloadID(blobName));
    }

    Option<ReadBlob> getReadBlob(std::string_view blobName) {
        const ReloadID id = getReloadID(blobName);
        auto it = _blobs.find(id);

        if (it == _blobs.end()) {
            return Option<ReadBlob>::none();
        }

        const SectorMeta& sector = it->second;

        return ReadBlob(std::span<std::uint8_t>(_dataBlob.data() + sector.start, sector.length));
    }

   private:
    struct SectorMeta {
        std::size_t start;
        std::size_t length;
    };

    static ReloadID getReloadID(std::string_view name) {
        return std::hash<std::string_view>{}(name);
    }

    void submitWriteBlob(const WriteBlob& writeBlob) {
        const std::size_t start = _dataBlob.size();
        const std::size_t size = writeBlob.blob.size();

        _dataBlob.resize(start + size);

        if (size > 0) {
            std::memcpy(_dataBlob.data() + start, writeBlob.blob.data(), size);
        }

        _blobs[writeBlob.id] = SectorMeta{
            .start = start,
            .length = size,
        };
    }

    std::vector<std::uint8_t> _dataBlob;
    std::map<ReloadID, SectorMeta> _blobs;
};

}  // namespace okay

#endif  // __RELOAD_HPP__
