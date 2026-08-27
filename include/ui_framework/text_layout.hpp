#pragma once

#include <cstddef>
#include <string>
#include <utility>

#include <SDL3_ttf/SDL_ttf.h>

#include "ui_framework/types.hpp"
#include "ui_framework/text_layout_result.hpp"

namespace ui
{
    enum class WrapMode
    {
        WRAP,
        NO_WRAP
    };

    // Shared logical text layout state used by standalone text components and
    // text-bearing controls. It measures in framework logical coordinates;
    // rendering/rasterization belongs to the backend layer.
    class TextLayout
    {
    public:
        TextLayout() = default;
        TextLayout(const TextLayout &) = default;
        TextLayout &operator=(const TextLayout &) = default;

        void setText(std::string text) { text_ = std::move(text); }
        const std::string &getText() const noexcept { return text_; }

        void setFont(TTF_Font *font) noexcept { font_ = font; }
        TTF_Font *getFont() const noexcept { return font_; }

        void setFontSize(float logicalSize) noexcept { fontSize_ = logicalSize; }
        float getFontSize() const noexcept { return fontSize_; }

        void setLineHeight(float logicalLineHeight) noexcept { lineHeight_ = logicalLineHeight; }
        float getLineHeight() const noexcept { return lineHeight_; }

        void setWrapMode(WrapMode mode) noexcept { wrapMode_ = mode; }
        WrapMode getWrapMode() const noexcept { return wrapMode_; }

        TextLayoutResult measureLayout(float availableWidth) const noexcept;
        LayoutSize measure(float availableWidth) const noexcept;

        // Logical text geometry for the current unwrapped single-line text.
        // The position is a UTF-8 code-point index, not a byte offset.
        float caretOffset(std::size_t textPosition) const noexcept;
        std::size_t textPositionAt(float x) const noexcept;

    private:
        std::string text_;
        TTF_Font *font_ = nullptr;
        float fontSize_ = 0.0f;
        float lineHeight_ = 0.0f;
        WrapMode wrapMode_ = WrapMode::WRAP;
    };
}
