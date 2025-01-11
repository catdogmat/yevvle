#pragma once

#include <cstdint>
#include <array>

namespace HW {

namespace Touch {
    constexpr std::array<uint8_t, 4> Pin = {{2,4,12,14}};
    // Layout of pads are TR=0, TL=2, BL=5, BR=6
    // Default map to DOWN=BR=6, UP=TR=0, MENU=TL=2, BACK=BL=5
    enum PadNames {BotR, TopR, TopL, BotL};
    constexpr std::array<uint8_t, 4> Pad = {{6,0,2,5}};
}

namespace DisplayPin {
    constexpr uint8_t Cs = 5;
    constexpr uint8_t Res = 9;
    constexpr uint8_t Dc = 10;
    constexpr uint8_t Sck = 18;
    constexpr uint8_t Busy = 19;
    constexpr uint8_t Mosi = 23;
}

constexpr uint8_t kAdcPin = 34;
constexpr uint8_t kBusyIntPin = 35; // On HW v3
constexpr uint8_t kRtcIntPin = 32;
constexpr uint8_t kLightPin = 25;
constexpr uint8_t kSpeakerPin = 26;
constexpr uint8_t kVibratorPin = 27;
constexpr uint8_t kVoltageSelectPin = 13;

// constexpr uint8_t kRevision = 1; // Original
constexpr uint8_t kRevision = 2; // New 2024/Oct / Swap PowerLevels
// constexpr uint8_t kRevision = 3; // New 2024/Nov / Add Busy Interrupt
// constexpr uint8_t kRevision = 4; // New 2025 / Complete LoRa redesign + GPS


} // namespace HW