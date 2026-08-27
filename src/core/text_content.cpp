#include "text_content.hpp"

#include <utility>

#include "text_renderer.hpp"

namespace ui
{
    TextContent::TextContent()
        : renderer_(std::make_unique<TextRenderer>())
    {
    }

    TextContent::~TextContent() = default;

    const std::string &TextContent::getText() const noexcept { return layout_.getText(); }
    void TextContent::setText(std::string text) { layout_.setText(std::move(text)); }
    TTF_Font *TextContent::getFont() const noexcept { return layout_.getFont(); }
    void TextContent::setFont(TTF_Font *font) noexcept { layout_.setFont(font); }
    float TextContent::getFontSize() const noexcept { return layout_.getFontSize(); }
    void TextContent::setFontSize(float logicalSize) noexcept { layout_.setFontSize(logicalSize); }
    float TextContent::getLineHeight() const noexcept { return layout_.getLineHeight(); }
    void TextContent::setLineHeight(float logicalLineHeight) noexcept { layout_.setLineHeight(logicalLineHeight); }
    WrapMode TextContent::getWrapMode() const noexcept { return layout_.getWrapMode(); }
    void TextContent::setWrapMode(WrapMode mode) noexcept { layout_.setWrapMode(mode); }
    TextAlignment TextContent::getHorizontalAlignment() const noexcept { return horizontalAlignment_; }
    void TextContent::setHorizontalAlignment(TextAlignment alignment) noexcept { horizontalAlignment_ = alignment; }
    TextAlignment TextContent::getVerticalAlignment() const noexcept { return verticalAlignment_; }
    void TextContent::setVerticalAlignment(TextAlignment alignment) noexcept { verticalAlignment_ = alignment; }
    Color TextContent::getColor() const noexcept { return color_; }
    void TextContent::setColor(const Color &color) noexcept { color_ = color; }

    LayoutSize TextContent::measure(float availableWidth) const
    {
        return layout_.measure(availableWidth);
    }

    void TextContent::arrange(const LayoutPosition &contentPosition, const LayoutSize &contentSize)
    {
        arrangedSize_ = contentSize;
        arrangedLayoutResult_ = layout_.measureLayout(contentSize.width);
        arrangedPosition_ = contentPosition;

        const LayoutSize textSize = arrangedLayoutResult_.desiredSize;

        switch (horizontalAlignment_)
        {
        case TextAlignment::CENTER:
            arrangedPosition_.x += (contentSize.width - textSize.width) * 0.5f;
            break;
        case TextAlignment::END:
            arrangedPosition_.x += contentSize.width - textSize.width;
            break;
        case TextAlignment::START:
        default:
            break;
        }

        switch (verticalAlignment_)
        {
        case TextAlignment::CENTER:
            arrangedPosition_.y += (contentSize.height - textSize.height) * 0.5f;
            break;
        case TextAlignment::END:
            arrangedPosition_.y += contentSize.height - textSize.height;
            break;
        case TextAlignment::START:
        default:
            break;
        }
    }

    void TextContent::draw(SDL_Renderer *renderer)
    {
        if (!renderer_ || layout_.getText().empty() || !layout_.getFont())
            return;

        renderer_->draw(
            renderer,
            layout_.getText(),
            layout_.getFont(),
            arrangedPosition_,
            arrangedLayoutResult_.wrapWidth,
            color_);
    }

    float TextContent::caretOffset(std::size_t textPosition) const noexcept
    {
        return layout_.caretOffset(textPosition);
    }

    std::size_t TextContent::textPositionAt(float x) const noexcept
    {
        return layout_.textPositionAt(x);
    }

    LayoutPosition TextContent::getArrangedPosition() const noexcept
    {
        return arrangedPosition_;
    }
}
