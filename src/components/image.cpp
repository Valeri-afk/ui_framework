#include "ui_framework/components/image.hpp"

#include <algorithm>

namespace ui
{
    namespace
    {
        LayoutSize queryTextureSize(SDL_Texture *texture) noexcept
        {
            if (!texture)
                return {};

            float width = 0.0f;
            float height = 0.0f;
            if (!SDL_GetTextureSize(texture, &width, &height))
                return {};

            return {width, height};
        }
    }

    void Image::setTexture(SDL_Texture *texture) noexcept
    {
        texture_ = texture;
        intrinsicSize_ = texture_ ? queryTextureSize(texture_) : LayoutSize{};
    }

    SDL_Texture *Image::getTexture() const noexcept
    {
        return texture_;
    }

    void Image::setIntrinsicSize(const LayoutSize &size) noexcept
    {
        intrinsicSize_.width = std::max(0.0f, size.width);
        intrinsicSize_.height = std::max(0.0f, size.height);
    }

    LayoutSize Image::getIntrinsicSize() const noexcept
    {
        return intrinsicSize_;
    }

    void Image::setFitMode(FitMode mode) noexcept
    {
        fitMode_ = mode;
    }

    Image::FitMode Image::getFitMode() const noexcept
    {
        return fitMode_;
    }

    void Image::setTint(Color color) noexcept
    {
        tint_ = color;
    }

    Color Image::getTint() const noexcept
    {
        return tint_;
    }

    LayoutSize Image::measureContent(const LayoutSize &) const
    {
        // Fit mode affects rendering inside the arranged box. It must not
        // turn an auto-sized image into a fill-parent element.
        return intrinsicSize_;
    }

    void Image::arrangeContent(const LayoutPosition &contentPosition, const LayoutSize &contentSize)
    {
        arrangedPosition_ = contentPosition;
        arrangedSize_ = contentSize;
    }

    void Image::draw(SDL_Renderer *renderer)
    {
        if (!renderer || !texture_ || arrangedSize_.width <= 0.0f || arrangedSize_.height <= 0.0f)
            return;

        Uint8 oldR = 255;
        Uint8 oldG = 255;
        Uint8 oldB = 255;
        Uint8 oldA = 255;
        const bool haveColorMod = SDL_GetTextureColorMod(texture_, &oldR, &oldG, &oldB);
        const bool haveAlphaMod = SDL_GetTextureAlphaMod(texture_, &oldA);

        SDL_SetTextureColorMod(texture_, tint_.r, tint_.g, tint_.b);
        SDL_SetTextureAlphaMod(texture_, tint_.a);

        const float sourceWidth = intrinsicSize_.width;
        const float sourceHeight = intrinsicSize_.height;
        if (sourceWidth <= 0.0f || sourceHeight <= 0.0f)
        {
            if (haveColorMod)
                SDL_SetTextureColorMod(texture_, oldR, oldG, oldB);
            if (haveAlphaMod)
                SDL_SetTextureAlphaMod(texture_, oldA);
            return;
        }

        SDL_FRect sourceRect{0.0f, 0.0f, sourceWidth, sourceHeight};
        SDL_FRect destinationRect{
            arrangedPosition_.x,
            arrangedPosition_.y,
            arrangedSize_.width,
            arrangedSize_.height};

        if (fitMode_ == FitMode::CONTAIN || fitMode_ == FitMode::COVER)
        {
            const float sourceAspect = sourceWidth / sourceHeight;
            const float destinationAspect = arrangedSize_.width / arrangedSize_.height;

            if (fitMode_ == FitMode::CONTAIN)
            {
                const float scale = destinationAspect < sourceAspect
                    ? arrangedSize_.width / sourceWidth
                    : arrangedSize_.height / sourceHeight;
                destinationRect.w = sourceWidth * scale;
                destinationRect.h = sourceHeight * scale;
                destinationRect.x += (arrangedSize_.width - destinationRect.w) * 0.5f;
                destinationRect.y += (arrangedSize_.height - destinationRect.h) * 0.5f;
            }
            else
            {
                const float scale = destinationAspect > sourceAspect
                    ? arrangedSize_.width / sourceWidth
                    : arrangedSize_.height / sourceHeight;
                const float visibleWidth = arrangedSize_.width / scale;
                const float visibleHeight = arrangedSize_.height / scale;
                sourceRect.x = (sourceWidth - visibleWidth) * 0.5f;
                sourceRect.y = (sourceHeight - visibleHeight) * 0.5f;
                sourceRect.w = visibleWidth;
                sourceRect.h = visibleHeight;
            }
        }

        SDL_RenderTexture(renderer, texture_, &sourceRect, &destinationRect);

        if (haveColorMod)
            SDL_SetTextureColorMod(texture_, oldR, oldG, oldB);
        if (haveAlphaMod)
            SDL_SetTextureAlphaMod(texture_, oldA);
    }
}
