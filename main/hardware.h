#pragma once

#include <cstdint>
#include <array>
#include <sdkconfig.h>

#if CONFIG_IDF_TARGET_ESP32S3
#define HW_VERSION 10 // First WIP board
#else
#define HW_VERSION 2 // Last board fully working
#endif

enum class HW_chips {ESP_32, ESP_32_S3};

// Original ESP32-PICO-D4 board based off from Watchy
// there was a previous 2 revisions with wrong wiring/touch/chip
// that I am not even going to ever cover with code
struct HW_1 {
    constexpr static uint8_t kVersion = 1;
    constexpr static HW_chips kChipType = HW_chips::ESP_32;
    constexpr static bool kHasLowVoltage = true;
    constexpr static bool kVoltageSelectInverted = false;
    constexpr static bool kHasLora = false;
    constexpr static bool kHasGps = false;
    constexpr static bool kHasDisplayBusyWake = false;

    struct Touch {
        enum PadNames {BotR /*Down*/, TopR /*Up*/, TopL /*Menu*/, BotL /*Back*/};
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
    // TODO: DUMMY remove me
    struct Gps {
        constexpr static uint8_t Tx = 0;
        constexpr static uint8_t Rx = 0;
        constexpr static uint8_t Vcc = 0;
        constexpr static auto BaudRate = 115200;
    };

    constexpr static uint8_t kAdcPin = 34;
    constexpr static uint8_t kLightPin = 25;
    constexpr static uint8_t kSpeakerPin = 26;
    constexpr static uint8_t kVibratorPin = 27;
    constexpr static uint8_t kVoltageSelectPin = 13;

    // Unused but available in V1 and V2
    // constexpr static uint8_t kRtcIntPin = 32;
    // constexpr static uint8_t kSdaPin = ??;
    // constexpr static uint8_t kSclPin = ??;
};

// Minor changes done to the board
struct HW_2 : public HW_1 {
    constexpr static uint8_t kVersion = 2;
    constexpr static bool kVoltageSelectInverted = true;
};

// Added Lora/GPS to PICOD4, reshuffle pins
struct HW_3 : public HW_2 {
    constexpr static uint8_t kVersion = 3;
    constexpr static bool kHasLowVoltage = true;
    constexpr static bool kHasLora = true;
    constexpr static bool kHasGps = true;
    constexpr static bool kHasDisplayBusyWake = true;


    // Use as much as possible INPUT ONLY pins 34-39
    struct Display {
        constexpr static uint8_t Cs = 5;
        constexpr static uint8_t Res = 9;
        constexpr static uint8_t Dc = 10;
        constexpr static uint8_t Sck = 18;
        constexpr static uint8_t Busy = 35; // Input only pin
        constexpr static uint8_t Mosi = 23;
    };

    struct Lora {
        //constexpr static uint8_t Cs = ?;
        //constexpr static uint8_t Res = ?;
        constexpr static uint8_t Dc = Display::Dc; // Shared
        constexpr static uint8_t Sck = Display::Sck; // Shared
        constexpr static uint8_t Busy = 37; // Input only pin
        constexpr static uint8_t Mosi = Display::Mosi; // Shared
        constexpr static uint8_t Miso = 38; // Input only pin
        constexpr static uint8_t Dio1 = 36; // IRQ? // Input only pin
        constexpr static uint8_t Dio2 = 32;
        //constexpr static uint8_t Vcc = ?; // Can it share with RES?
    };

    struct Gps {
        //constexpr static uint8_t Tx = ??;
        constexpr static uint8_t Rx = 39; // Input only pin
        //constexpr static uint8_t Vcc = ??;
    };

};


// New board based on ESP32-S3-FN8
struct HW_10 {
    constexpr static uint8_t kVersion = 10;
    constexpr static HW_chips kChipType = HW_chips::ESP_32_S3;
    // Sadly the feature makes the SoC off...
    constexpr static bool kHasLowVoltage = false;
    constexpr static bool kVoltageSelectInverted = true /*false*/; // TEMPORARY
    constexpr static bool kHasLora = true;
    constexpr static bool kHasGps = true;
    constexpr static bool kHasDisplayBusyWake = true;

    struct Touch {
        enum PadNames {BotR /*Down*/, TopR /*Up*/, TopL /*Menu*/, BotL /*Back*/};
        constexpr static std::array<uint8_t, 4> Pad = {{1,4,8,6}};
    };

    struct Display {
        constexpr static uint8_t Cs = 20;
        constexpr static uint8_t Res = 10; // NOT ALLOWED 22
        constexpr static uint8_t Dc = 21;
        constexpr static uint8_t Sck = 19;
        constexpr static uint8_t Busy = 7;
        constexpr static uint8_t Mosi = 18;
    };

    struct Lora {
        constexpr static uint8_t Cs = 35;
        //constexpr static uint8_t Res = ?;
        constexpr static uint8_t Dc = Display::Dc; // Shared
        constexpr static uint8_t Sck = Display::Sck; // Shared
        constexpr static uint8_t Busy = 36; // Input only pin
        constexpr static uint8_t Mosi = Display::Mosi; // Shared
        constexpr static uint8_t Miso = 17; // Input only pin
        constexpr static uint8_t Dio1 = 34; // IRQ? // Input only pin
        constexpr static uint8_t Dio2 = 33;
        //constexpr static uint8_t Vcc = ?; // Can it share with RES?
    };

    struct Gps {
        constexpr static uint8_t Tx = 38;
        constexpr static uint8_t Rx = 37;
        constexpr static uint8_t Vcc = 11;
        constexpr static auto BaudRate = 115200;
    };

    constexpr static uint8_t kAdcPin = 9;
    // constexpr static uint8_t kRtcIntPin = 1;
    constexpr static uint8_t kLightPin = 12;
    constexpr static uint8_t kSpeakerPin = 2;
    constexpr static uint8_t kVibratorPin = 14;
    constexpr static uint8_t kVoltageSelectPin = 5;
};

#define CONCAT(a, b) a##b
#define HW_TYPE(version) CONCAT(HW_, version)

using HW = HW_TYPE(HW_VERSION);