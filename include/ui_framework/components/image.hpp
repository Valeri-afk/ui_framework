#pragma once

#include <SDL3/SDL.h>

#include "ui_framework/node.hpp"
#include "ui_framework/types.hpp"

namespace ui
{
    class Image : public Node
    {
    public:
        enum class FitMode
        {
            STRETCH,
            CONTAIN,
            COVER
        };

        Image() = default;
        ~Image() override = default;

        Image(const Image &) = delete;
        Image &operator=(const Image &) = delete;

        void setTexture(SDL_Texture *texture) noexcept;
        SDL_Texture *getTexture() const noexcept;

        void setIntrinsicSize(const LayoutSize &size) noexcept;
        LayoutSize getIntrinsicSize() const noexcept;

        void setFitMode(FitMode mode) noexcept;
        FitMode getFitMode() const noexcept;

        void setTint(Color color) noexcept;
        Color getTint() const noexcept;

    protected:
        LayoutSize measureContent(const LayoutSize &availableContent) const override;
        void arrangeContent(const LayoutPosition &contentPosition, const LayoutSize &contentSize) override;
        void draw(SDL_Renderer *renderer) override;

    private:
        SDL_Texture *texture_ = nullptr;
        LayoutSize intrinsicSize_{};
        LayoutPosition arrangedPosition_{};
        LayoutSize arrangedSize_{};
        FitMode fitMode_ = FitMode::CONTAIN;
        Color tint_ = Colors::white;
    };
}
