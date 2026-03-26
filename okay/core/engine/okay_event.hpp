#ifndef __OKAY_EVENT_H__
#define __OKAY_EVENT_H__

#include <functional>

namespace okay {

template <typename... Args>
class OkayEvent {
    using ListenerFn = std::function<void(Args...)>;

   public:
    void subscribe(ListenerFn listener) { _listeners.push_back(listener); }
    void unsubscribe(ListenerFn listener) {
        _listeners.erase(std::remove(_listeners.begin(), _listeners.end(), listener),
                         _listeners.end());
    }
    void emit(Args... args) const {
        for (const auto& listener : _listeners)
            listener(args...);
    }

   private:
    std::vector<ListenerFn> _listeners;
};

};  // namespace okay

#endif  // __OKAY_EVENT_H__