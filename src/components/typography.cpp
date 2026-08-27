#include "ui_framework/components/typography.hpp"
#include "../core/text_content.hpp"

#include <algorithm>
#include <utility>

namespace ui
{
    namespace
    {
        float defaultFontSize(TypographyVariant variant) noexcept
        {
            switch (variant)
            {
            case TypographyVariant::H1: return 32.0f;
            case TypographyVariant::H2: return 28.0f;
            case TypographyVariant::H3: return 24.0f;
            case TypographyVariant::H4: return 20.0f;
            case TypographyVariant::H5: return 18.0f;
            case TypographyVariant::H6: return 16.0f;
            case TypographyVariant::SUBTITLE1: return 16.0f;
            case TypographyVariant::SUBTITLE2: return 14.0f;
            case TypographyVariant::BODY1: return 16.0f;
            case TypographyVariant::BODY2: return 14.0f;
            case TypographyVariant::BUTTON: return 14.0f;
            case TypographyVariant::CAPTION: return 12.0f;
            case TypographyVariant::OVERLINE: return 12.0f;
            case TypographyVariant::INHERIT: default: return 16.0f;
            }
        }
    }

    Typography::Typography() : text_(std::make_unique<TextContent>())
    {
        applyVariantDefaults();
    }

    Typography::~Typography() = default;

    const std::string &Typography::getText() const noexcept { return text_->getText(); }
    void Typography::setText(std::string text) { text_->setText(std::move(text)); }
    TTF_Font *Typography::getFont() const noexcept { return text_->getFont(); }
    void Typography::setFont(TTF_Font *font) { text_->setFont(font); }
    void Typography::setVariant(Variant variant) noexcept { if (variant_ == variant) return; variant_ = variant; applyVariantDefaults(); }
    Typography::Variant Typography::getVariant() const noexcept { return variant_; }
    void Typography::setFontSize(float logicalSize) noexcept { explicitFontSize_ = std::max(0.0f, logicalSize); fontSizeExplicit_ = true; text_->setFontSize(explicitFontSize_); }
    float Typography::getFontSize() const noexcept { return text_->getFontSize(); }
    void Typography::setLineHeight(float logicalLineHeight) noexcept { explicitLineHeight_ = std::max(0.0f, logicalLineHeight); lineHeightExplicit_ = true; text_->setLineHeight(explicitLineHeight_); }
    float Typography::getLineHeight() const noexcept { return text_->getLineHeight(); }
    void Typography::setWrapMode(WrapMode mode) noexcept { text_->setWrapMode(mode); }
    WrapMode Typography::getWrapMode() const noexcept { return text_->getWrapMode(); }
    TextAlignment Typography::getHorizontalAlignment() const noexcept { return text_->getHorizontalAlignment(); }
    void Typography::setHorizontalAlignment(TextAlignment alignment) { text_->setHorizontalAlignment(alignment); }
    TextAlignment Typography::getVerticalAlignment() const noexcept { return text_->getVerticalAlignment(); }
    void Typography::setVerticalAlignment(TextAlignment alignment) { text_->setVerticalAlignment(alignment); }
    Color Typography::getColor() const noexcept { return text_->getColor(); }
    void Typography::setColor(const Color &color) { text_->setColor(color); }
    LayoutSize Typography::measureContent(const LayoutSize &availableContent) const { return text_->measure(availableContent.width); }
    void Typography::arrangeContent(const LayoutPosition &contentPosition, const LayoutSize &contentSize) { text_->arrange(contentPosition, contentSize); }
    void Typography::draw(SDL_Renderer *renderer) { text_->draw(renderer); }

    void Typography::applyVariantDefaults() noexcept
    {
        if (!fontSizeExplicit_) text_->setFontSize(defaultFontSize(variant_));
        else text_->setFontSize(explicitFontSize_);
        if (!lineHeightExplicit_) text_->setLineHeight(0.0f);
        else text_->setLineHeight(explicitLineHeight_);
    }
}
