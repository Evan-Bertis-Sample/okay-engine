#ifndef _EVENT_H__
#define _EVENT_H__

#include <functional>

namespace okay {

template <typename... Args>
class Event {
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

#endif  // _EVENT_H__