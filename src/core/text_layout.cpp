#include "ui_framework/text_layout.hpp"

#include <algorithm>
#include <cmath>
#include <string_view>

namespace ui
{
    namespace
    {
        class MeasureFont final
        {
        public:
            explicit MeasureFont(TTF_Font *source, float requestedSize, float requestedLineHeight) noexcept
            {
                font_ = source;
                if (!source)
                    return;

                const float sourceSize = TTF_GetFontSize(source);
                const bool needsSize = requestedSize > 0.0f && sourceSize > 0.0f && std::abs(sourceSize - requestedSize) >= 0.001f;
                const int nativeLineSkip = TTF_GetFontLineSkip(source);
                const bool needsLineSkip = requestedLineHeight > 0.0f && std::abs(static_cast<float>(nativeLineSkip) - requestedLineHeight) >= 0.001f;

                if (!needsSize && !needsLineSkip)
                    return;

                TTF_Font *copy = TTF_CopyFont(source);
                if (!copy)
                {
                    font_ = nullptr;
                    return;
                }

                if (needsSize && !TTF_SetFontSize(copy, requestedSize))
                {
                    TTF_CloseFont(copy);
                    font_ = nullptr;
                    return;
                }

                if (requestedLineHeight > 0.0f)
                    TTF_SetFontLineSkip(copy, std::max(1, static_cast<int>(std::lround(requestedLineHeight))));

                owned_ = copy;
                font_ = copy;
            }

            ~MeasureFont()
            {
                if (owned_)
                    TTF_CloseFont(owned_);
            }

            TTF_Font *get() const noexcept { return font_; }
            MeasureFont(const MeasureFont &) = delete;
            MeasureFont &operator=(const MeasureFont &) = delete;

        private:
            TTF_Font *font_ = nullptr;
            TTF_Font *owned_ = nullptr;
        };

        std::size_t utf8CodePointCount(std::string_view text) noexcept
        {
            std::size_t count = 0;
            for (std::size_t i = 0; i < text.size();)
            {
                const unsigned char c = static_cast<unsigned char>(text[i]);
                std::size_t length = 1;
                if ((c & 0x80u) == 0)
                    length = 1;
                else if ((c & 0xE0u) == 0xC0u)
                    length = 2;
                else if ((c & 0xF0u) == 0xE0u)
                    length = 3;
                else if ((c & 0xF8u) == 0xF0u)
                    length = 4;

                if (i + length > text.size())
                    length = 1;

                i += length;
                ++count;
            }
            return count;
        }

        std::size_t utf8ByteOffset(std::string_view text, std::size_t position) noexcept
        {
            std::size_t index = 0;
            std::size_t byte = 0;
            while (byte < text.size() && index < position)
            {
                const unsigned char c = static_cast<unsigned char>(text[byte]);
                std::size_t length = 1;
                if ((c & 0x80u) == 0)
                    length = 1;
                else if ((c & 0xE0u) == 0xC0u)
                    length = 2;
                else if ((c & 0xF0u) == 0xE0u)
                    length = 3;
                else if ((c & 0xF8u) == 0xF0u)
                    length = 4;

                if (byte + length > text.size())
                    length = 1;

                byte += length;
                ++index;
            }
            return byte;
        }
    }

    TextLayoutResult TextLayout::measureLayout(float availableWidth) const noexcept
    {
        TextLayoutResult result{};
        if (!font_ || text_.empty())
            return result;

        MeasureFont measureFont(font_, fontSize_, lineHeight_);
        TTF_Font *font = measureFont.get();
        if (!font)
            return result;

        int width = 0;
        int height = 0;
        if (wrapMode_ == WrapMode::WRAP && availableWidth > 0.0f)
        {
            result.wrapWidth = std::max(1.0f, std::floor(availableWidth));
            const int wrapWidth = static_cast<int>(result.wrapWidth);
            if (!TTF_GetStringSizeWrapped(font, text_.c_str(), 0, wrapWidth, &width, &height))
                return result;
        }
        else
        {
            result.wrapWidth = 0.0f;
            if (!TTF_GetStringSize(font, text_.c_str(), 0, &width, &height))
                return result;
        }

        result.desiredSize = {
            static_cast<float>(std::max(width, 0)),
            static_cast<float>(std::max(height, 0))
        };
        result.lineHeight = static_cast<float>(TTF_GetFontLineSkip(font));
        result.lineCount = result.lineHeight > 0.0f
            ? std::max(1, static_cast<int>(std::lround(result.desiredSize.height / result.lineHeight)))
            : 1;
        return result;
    }

    LayoutSize TextLayout::measure(float availableWidth) const noexcept
    {
        return measureLayout(availableWidth).desiredSize;
    }

    float TextLayout::caretOffset(std::size_t textPosition) const noexcept
    {
        if (!font_ || text_.empty())
            return 0.0f;

        const std::size_t count = utf8CodePointCount(text_);
        textPosition = std::min(textPosition, count);
        const std::size_t byteOffset = utf8ByteOffset(text_, textPosition);
        const std::string prefix = text_.substr(0, byteOffset);

        MeasureFont measureFont(font_, fontSize_, lineHeight_);
        TTF_Font *font = measureFont.get();
        if (!font)
            return 0.0f;

        int width = 0;
        int height = 0;
        if (!TTF_GetStringSize(font, prefix.c_str(), 0, &width, &height))
            return 0.0f;
        return static_cast<float>(std::max(width, 0));
    }

    std::size_t TextLayout::textPositionAt(float x) const noexcept
    {
        if (!font_ || text_.empty() || x <= 0.0f)
            return 0;

        const std::size_t count = utf8CodePointCount(text_);
        for (std::size_t i = 0; i < count; ++i)
        {
            const float left = caretOffset(i);
            const float right = caretOffset(i + 1);
            if (x < (left + right) * 0.5f)
                return i;
        }
        return count;
    }
}
