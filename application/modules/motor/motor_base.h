#pragma once

#include <cstdint>

class MotorBase {
public:
    enum class State : uint8_t { Stopped, Enabled, Error };

    virtual ~MotorBase() = default;
    virtual void enable() = 0;
    virtual void disable() = 0;

    State getState() const { return state_; }
    bool isEnabled() const { return state_ == State::Enabled; }

protected:
    State state_ = State::Stopped;
};
