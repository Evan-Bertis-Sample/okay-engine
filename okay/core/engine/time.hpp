#ifndef __TIME_H__
#define __TIME_H__

#include <chrono>

namespace okay {

class OkayTime {
    using HighResClock = std::chrono::high_resolution_clock;
    using TimePoint = std::chrono::time_point<HighResClock>;

   public:
    OkayTime()
        : _lastTime(HighResClock::now()), _startOfProgram(HighResClock::now()), _deltaTime(0) {}

    void reset() {
        _lastTime = HighResClock::now();
        _deltaTime = 0;
    }

    std::uint32_t deltaTimeMs() const { return _deltaTime; }
    float deltaTimeSec() const { return static_cast<float>(_deltaTime) / 1000.0f; }

    std::uint32_t timeSinceStartMs() const {
        return std::chrono::duration_cast<std::chrono::milliseconds>(HighResClock::now() -
                                                                     _startOfProgram)
            .count();
    }
    float timeSinceStartSec() const { return static_cast<float>(timeSinceStartMs()) / 1000.0f; }

    void updateDeltaTime() {
        TimePoint prevTime = _lastTime;
        _lastTime = HighResClock::now();
        _deltaTime =
            std::chrono::duration_cast<std::chrono::milliseconds>(_lastTime - prevTime).count();
    }

   private:
    TimePoint _lastTime;
    TimePoint _startOfProgram;
    std::uint32_t _deltaTime;
};

}  // namespace okay

#endif  // __TIME_H__