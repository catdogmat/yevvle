#pragma once

#include <cstdint>
#include <array>

#if CONFIG_IDF_TARGET_ESP32S3
#define HW_VERSION 3 // First WIP board
#else
#define HW_VERSION 2 // Last board fully working
#endif

enum class HW_chips {ESP_32, ESP_32_S3};

// Original ESP32-PICO-D4 board based off from Watchy
// there was a previous 2 revisions with wrong wiring/touch/chip
// that I am not even going to ever cover with code
struct HW_1 {
    constexpr static uint8_t kVersion = 1;
    struct Touch {
        constexpr static std::array<uint8_t, 4> Pin = {{2,4,12,14}};
        // Layout of pads are TR=0, TL=2, BL=5, BR=6
        // Default map to DOWN=BR=6, UP=TR=0, MENU=TL=2, BACK=BL=5
        enum PadNames {BotR, TopR, TopL, BotL};
        constexpr static std::array<uint8_t, 4> Pad = {{6,0,2,5}};
    };

    struct Display {
        constexpr static uint8_t Cs = 5;
        constexpr static uint8_t Res = 9;
        constexpr static uint8_t Dc = 10;
        constexpr static uint8_t Sck = 18;
        constexpr static uint8_t Busy = 19; // 35 better, never manufactured
        constexpr static uint8_t Mosi = 23;
    };

    constexpr static uint8_t kAdcPin = 34;
    constexpr static uint8_t kRtcIntPin = 32;
    constexpr static uint8_t kLightPin = 25;
    constexpr static uint8_t kSpeakerPin = 26;
    constexpr static uint8_t kVibratorPin = 27;
    constexpr static uint8_t kVoltageSelectPin = 13;

    constexpr static HW_chips kChipType = HW_chips::ESP_32;
    constexpr static bool kVoltageSelectInverted = false;
};

// Minor changes done to the board, last board with ESP32-PICO-D4
struct HW_2 : public HW_1 {
    constexpr static uint8_t kVersion = 2;
    constexpr static bool kVoltageSelectInverted = true;
};

// New board based on ESP32-S3-FN8
struct HW_3 : public HW_2 {
    constexpr static uint8_t kVersion = 3;

    constexpr static HW_chips kChipType = HW_chips::ESP_32_S3;
};

#define CONCAT(a, b) a##b
#define HW_TYPE(version) CONCAT(HW_, version)

using HW = HW_TYPE(HW_VERSION);