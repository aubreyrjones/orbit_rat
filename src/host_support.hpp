#pragma once
#include <stdint.h>

enum class HostPacketType : uint8_t {
    HELLO = 1,
    LAYOUT,
    SWITCH_MODES,
    BORDER_BUMP
};

struct HostLayoutMessage {
    HostPacketType type;
    uint16_t width, height;
};

