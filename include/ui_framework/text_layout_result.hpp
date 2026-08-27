#pragma once

#include "ui_framework/types.hpp"

namespace ui
{
    struct TextLayoutResult
    {
        LayoutSize desiredSize{};
        float lineHeight = 0.0f;
        float wrapWidth = 0.0f;
        int lineCount = 0;
    };
}
