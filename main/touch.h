#pragma once

#include <array>
#include <cstdint> 

#include <magic_enum.hpp>

enum MeasureRate {
    _125ms = 1, // Extremely fast
    _250ms = 2,
    _500ms = 4,
    _1s = 8,
    _2s = 16,
    // _4s = 32, // Not possible, limit is 2s
};

enum MeasureCycles {
    _31ms = 1,
    // _62ms = 2,
};

struct TouchSettings {
    // More cycles more accurate, and more power
    // More often checks, also more power
    MeasureCycles mCycles[2] = {_31ms};
    MeasureRate mRate[2] = {_250ms, _2s};

    std::array<uint8_t, 4> mThresholds{{30, 30, 30, 30}};
    std::array<uint8_t, 4> mMap{{0,1,2,3}};

    bool mSetup : 1 {false};
    bool mSetupMode : 1 {false};
};

class Touch {
private:
    TouchSettings& mSettings;
public:
    explicit Touch(TouchSettings& settings) : mSettings{settings} {};

    void setUp(bool onlyMenuLight);

    enum Btn {
        NONE = 0,
        DOWN = 0b0001,
        UP   = 0b0010,
        MENU = 0b0100,
        BACK = 0b1000,
        // Alias
        LIGHT = BACK,
    };
    Btn read() const;
    void clear() const;
    Btn readAndClear() const;
};