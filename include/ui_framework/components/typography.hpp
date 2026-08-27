#pragma once

#include <memory>
#include <string>

#include <SDL3_ttf/SDL_ttf.h>

#include "ui_framework/node.hpp"
#include "ui_framework/typography.hpp"
#include "ui_framework/text_layout.hpp"

namespace ui
{
    class TextContent;

    class Typography : public Node
    {
    public:
        using Variant = TypographyVariant;

        Typography();
        ~Typography() override;
        Typography(const Typography &) = delete;
        Typography &operator=(const Typography &) = delete;

        const std::string &getText() const noexcept;
        void setText(std::string text);
        TTF_Font *getFont() const noexcept;
        void setFont(TTF_Font *font);

        void setVariant(Variant variant) noexcept;
        Variant getVariant() const noexcept;
        void setFontSize(float logicalSize) noexcept;
        float getFontSize() const noexcept;
        void setLineHeight(float logicalLineHeight) noexcept;
        float getLineHeight() const noexcept;
        void setWrapMode(WrapMode mode) noexcept;
        WrapMode getWrapMode() const noexcept;

        TextAlignment getHorizontalAlignment() const noexcept;
        void setHorizontalAlignment(TextAlignment alignment);
        TextAlignment getVerticalAlignment() const noexcept;
        void setVerticalAlignment(TextAlignment alignment);
        Color getColor() const noexcept;
        void setColor(const Color &color);

    protected:
        LayoutSize measureContent(const LayoutSize &availableContent) const override;
        void arrangeContent(const LayoutPosition &contentPosition, const LayoutSize &contentSize) override;
        void draw(SDL_Renderer *renderer) override;

    private:
        void applyVariantDefaults() noexcept;

        std::unique_ptr<TextContent> text_;
        Variant variant_ = Variant::BODY1;
        bool fontSizeExplicit_ = false;
        float explicitFontSize_ = 0.0f;
        bool lineHeightExplicit_ = false;
        float explicitLineHeight_ = 0.0f;
    };
}
