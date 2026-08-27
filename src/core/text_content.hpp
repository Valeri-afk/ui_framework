#pragma once

#include <cstddef>
#include <memory>
#include <string>

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include "ui_framework/text_layout.hpp"
#include "ui_framework/types.hpp"

namespace ui
{
    class TextRenderer;

    class TextContent
    {
    public:
        TextContent();
        ~TextContent();
        TextContent(const TextContent &) = delete;
        TextContent &operator=(const TextContent &) = delete;

        const std::string &getText() const noexcept;
        void setText(std::string text);
        TTF_Font *getFont() const noexcept;
        void setFont(TTF_Font *font) noexcept;
        float getFontSize() const noexcept;
        void setFontSize(float logicalSize) noexcept;
        float getLineHeight() const noexcept;
        void setLineHeight(float logicalLineHeight) noexcept;
        WrapMode getWrapMode() const noexcept;
        void setWrapMode(WrapMode mode) noexcept;
        TextAlignment getHorizontalAlignment() const noexcept;
        void setHorizontalAlignment(TextAlignment alignment) noexcept;
        TextAlignment getVerticalAlignment() const noexcept;
        void setVerticalAlignment(TextAlignment alignment) noexcept;
        Color getColor() const noexcept;
        void setColor(const Color &color) noexcept;

        LayoutSize measure(float availableWidth) const;
        void arrange(const LayoutPosition &contentPosition, const LayoutSize &contentSize);
        void draw(SDL_Renderer *renderer);

        float caretOffset(std::size_t textPosition) const noexcept;
        std::size_t textPositionAt(float x) const noexcept;
        LayoutPosition getArrangedPosition() const noexcept;

    private:
        TextLayout layout_;
        TextAlignment horizontalAlignment_ = TextAlignment::START;
        TextAlignment verticalAlignment_ = TextAlignment::START;
        Color color_ = Colors::white;
        LayoutPosition arrangedPosition_{};
        LayoutSize arrangedSize_{};
        TextLayoutResult arrangedLayoutResult_{};
        std::unique_ptr<TextRenderer> renderer_;
    };
}
