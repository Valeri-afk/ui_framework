#include "text_renderer.hpp"

#include <algorithm>
#include <cmath>

namespace
{
    class PhysicalTextRenderScope final
    {
    public:
        PhysicalTextRenderScope(SDL_Renderer *renderer, int logicalWidth, int logicalHeight,
                                SDL_RendererLogicalPresentation mode, const SDL_FRect &presentationRect,
                                float logicalScale) noexcept
            : renderer_(renderer), logicalWidth_(logicalWidth), logicalHeight_(logicalHeight),
              mode_(mode), presentationRect_(presentationRect), logicalScale_(logicalScale)
        {
            if (!renderer_ || logicalScale_ <= 0.0f)
                return;

            viewportSet_ = SDL_RenderViewportSet(renderer_);
            hasViewport_ = SDL_GetRenderViewport(renderer_, &viewport_);
            clipEnabled_ = SDL_RenderClipEnabled(renderer_);
            hasClip_ = SDL_GetRenderClipRect(renderer_, &clip_);
            hasScale_ = SDL_GetRenderScale(renderer_, &scaleX_, &scaleY_);

            if (!SDL_SetRenderLogicalPresentation(renderer_, 0, 0, SDL_LOGICAL_PRESENTATION_DISABLED))
                return;

            active_ = true;

            if (viewportSet_ && hasViewport_)
            {
                SDL_Rect physical{};
                physical.x = static_cast<int>(std::floor(presentationRect_.x + viewport_.x * logicalScale_));
                physical.y = static_cast<int>(std::floor(presentationRect_.y + viewport_.y * logicalScale_));
                physical.w = static_cast<int>(std::ceil(viewport_.w * logicalScale_));
                physical.h = static_cast<int>(std::ceil(viewport_.h * logicalScale_));
                SDL_SetRenderViewport(renderer_, &physical);
            }
            else
            {
                SDL_SetRenderViewport(renderer_, nullptr);
            }

            if (clipEnabled_ && hasClip_)
            {
                SDL_Rect physical{};
                physical.x = static_cast<int>(std::floor(presentationRect_.x + clip_.x * logicalScale_));
                physical.y = static_cast<int>(std::floor(presentationRect_.y + clip_.y * logicalScale_));
                physical.w = static_cast<int>(std::ceil(clip_.w * logicalScale_));
                physical.h = static_cast<int>(std::ceil(clip_.h * logicalScale_));
                SDL_SetRenderClipRect(renderer_, &physical);
            }
            else
            {
                SDL_SetRenderClipRect(renderer_, nullptr);
            }

            SDL_SetRenderScale(renderer_, 1.0f, 1.0f);
        }

        ~PhysicalTextRenderScope()
        {
            if (!renderer_ || !active_)
                return;

            SDL_SetRenderLogicalPresentation(renderer_, logicalWidth_, logicalHeight_, mode_);
            if (viewportSet_ && hasViewport_)
                SDL_SetRenderViewport(renderer_, &viewport_);
            else
                SDL_SetRenderViewport(renderer_, nullptr);
            if (clipEnabled_ && hasClip_)
                SDL_SetRenderClipRect(renderer_, &clip_);
            else
                SDL_SetRenderClipRect(renderer_, nullptr);
            if (hasScale_)
                SDL_SetRenderScale(renderer_, scaleX_, scaleY_);
        }

        bool isActive() const noexcept { return active_; }

        PhysicalTextRenderScope(const PhysicalTextRenderScope &) = delete;
        PhysicalTextRenderScope &operator=(const PhysicalTextRenderScope &) = delete;

    private:
        SDL_Renderer *renderer_ = nullptr;
        int logicalWidth_ = 0;
        int logicalHeight_ = 0;
        SDL_RendererLogicalPresentation mode_ = SDL_LOGICAL_PRESENTATION_DISABLED;
        SDL_FRect presentationRect_{};
        float logicalScale_ = 1.0f;
        SDL_Rect viewport_{}, clip_{};
        float scaleX_ = 1.0f, scaleY_ = 1.0f;
        bool viewportSet_ = false, hasViewport_ = false;
        bool clipEnabled_ = false, hasClip_ = false, hasScale_ = false;
        bool active_ = false;
    };
}

namespace ui
{
    TextRenderer::~TextRenderer()
    {
        releaseTextObject();
        releaseRasterFont();
    }

    void TextRenderer::draw(SDL_Renderer *renderer, const std::string &text, TTF_Font *font,
                            const LayoutPosition &position, float wrapWidth, Color color)
    {
        if (!renderer || !font || text.empty())
            return;

        float scale = 1.0f;
        SDL_FRect presentationRect{};
        const bool usePhysicalText = getIntegerPresentationScale(renderer, scale, presentationRect) &&
                                      ensureRasterFont(font, scale);
        TTF_Font *renderFont = usePhysicalText ? rasterFont_ : font;

        if (!ensureTextObject(renderer, renderFont, text))
            return;

        const int physicalWrapWidth = wrapWidth > 0.0f
            ? std::max(1, static_cast<int>(std::floor(wrapWidth * scale)))
            : 0;
        if (!TTF_SetTextWrapWidth(textObject_, physicalWrapWidth))
            return;

        TTF_SetTextColor(textObject_, color.r, color.g, color.b, color.a);

        if (!usePhysicalText)
        {
            TTF_DrawRendererText(textObject_, position.x, position.y);
            return;
        }

        int logicalWidth = 0, logicalHeight = 0;
        SDL_RendererLogicalPresentation mode = SDL_LOGICAL_PRESENTATION_DISABLED;
        if (!SDL_GetRenderLogicalPresentation(renderer, &logicalWidth, &logicalHeight, &mode))
            return;

        PhysicalTextRenderScope scope(renderer, logicalWidth, logicalHeight, mode, presentationRect, scale);
        if (!scope.isActive())
            return;

        TTF_DrawRendererText(
            textObject_,
            presentationRect.x + position.x * scale,
            presentationRect.y + position.y * scale);
    }

    bool TextRenderer::getIntegerPresentationScale(SDL_Renderer *renderer, float &scale,
                                                     SDL_FRect &presentationRect) const noexcept
    {
        scale = 1.0f;
        presentationRect = {};
        if (!renderer)
            return false;

        int logicalWidth = 0, logicalHeight = 0;
        SDL_RendererLogicalPresentation mode = SDL_LOGICAL_PRESENTATION_DISABLED;
        if (!SDL_GetRenderLogicalPresentation(renderer, &logicalWidth, &logicalHeight, &mode) ||
            mode != SDL_LOGICAL_PRESENTATION_INTEGER_SCALE || logicalWidth <= 0 || logicalHeight <= 0 ||
            !SDL_GetRenderLogicalPresentationRect(renderer, &presentationRect) ||
            presentationRect.w <= 0.0f || presentationRect.h <= 0.0f)
            return false;

        const float scaleX = presentationRect.w / static_cast<float>(logicalWidth);
        const float scaleY = presentationRect.h / static_cast<float>(logicalHeight);
        if (scaleX <= 0.0f || scaleY <= 0.0f || std::abs(scaleX - scaleY) > 0.001f)
            return false;

        scale = scaleX;
        return scale > 1.0f;
    }

    bool TextRenderer::ensureRasterFont(TTF_Font *font, float scale)
    {
        if (!font || scale <= 1.0f)
            return false;

        const Uint32 generation = TTF_GetFontGeneration(font);
        if (rasterFont_ && rasterSourceFont_ == font && rasterScale_ == scale && rasterFontGeneration_ == generation)
            return true;

        releaseRasterFont();
        rasterFont_ = TTF_CopyFont(font);
        if (!rasterFont_)
            return false;

        const float rasterSize = TTF_GetFontSize(font) * scale;
        if (rasterSize <= 0.0f || !TTF_SetFontSize(rasterFont_, rasterSize))
        {
            releaseRasterFont();
            return false;
        }

        rasterSourceFont_ = font;
        rasterScale_ = scale;
        rasterFontGeneration_ = generation;
        return true;
    }

    void TextRenderer::releaseTextObject() noexcept
    {
        if (textObject_)
            TTF_DestroyText(textObject_);
        if (textEngine_)
            TTF_DestroyRendererTextEngine(textEngine_);
        textObject_ = nullptr;
        textEngine_ = nullptr;
        cachedRenderer_ = nullptr;
        cachedTextFont_ = nullptr;
        cachedText_.clear();
    }

    void TextRenderer::releaseRasterFont() noexcept
    {
        if (rasterFont_)
            TTF_CloseFont(rasterFont_);
        rasterFont_ = nullptr;
        rasterSourceFont_ = nullptr;
        rasterScale_ = 1.0f;
        rasterFontGeneration_ = 0;
    }

    bool TextRenderer::ensureTextObject(SDL_Renderer *renderer, TTF_Font *font, const std::string &text)
    {
        if (!renderer || !font)
            return false;

        if (cachedRenderer_ != renderer || cachedTextFont_ != font || cachedText_ != text)
            releaseTextObject();

        if (!textEngine_)
            textEngine_ = TTF_CreateRendererTextEngine(renderer);
        if (!textEngine_)
            return false;

        if (!textObject_)
            textObject_ = TTF_CreateText(textEngine_, font, text.c_str(), 0);
        if (!textObject_)
            return false;

        cachedRenderer_ = renderer;
        cachedTextFont_ = font;
        cachedText_ = text;
        return true;
    }
}
