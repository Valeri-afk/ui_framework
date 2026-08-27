#pragma once

#include <cstdint>

namespace ui
{
    struct Color
    {
        uint8_t r = 255;
        uint8_t g = 255;
        uint8_t b = 255;
        uint8_t a = 255;
        bool operator==(const Color &) const = default;
    };

    namespace Colors
    {
        inline constexpr Color green{0, 255, 0, 255};
        inline constexpr Color red{255, 100, 47, 255};
        inline constexpr Color yellow{255, 255, 0, 255};
        inline constexpr Color white{255, 255, 255, 255};
        inline constexpr Color blue{0, 0, 255, 255};
        inline constexpr Color black{0, 0, 0, 255};
        inline constexpr Color gray{128, 128, 128, 255};
        inline constexpr Color camel{181, 136, 99, 255};
        inline constexpr Color desertSand{240, 217, 181, 255};
        inline constexpr Color transparent{0, 0, 0, 0};
    }
}
