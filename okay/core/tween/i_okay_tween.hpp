// Copyright (c) 2026 Okay Engine contributors
// SPDX-License-Identifier: MIT
//
// i_okay_tween.hpp - Okay Engine

#pragma once

namespace okay {

/**
 * @brief Interface for OkayTween dependency injection
 */
class ITween {
   public:
    virtual void start() = 0;
    virtual void tick() = 0;
    virtual void pause() = 0;
    virtual void resume() = 0;
    virtual void reset() = 0;
    virtual void kill() = 0;
    virtual bool isFinished() = 0;
    virtual void setIsTweening(bool isTweening) = 0;
    virtual ~ITween() = default;
};

}  // namespace okay
