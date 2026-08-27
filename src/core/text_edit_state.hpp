#pragma once

#include <cstddef>
#include <string>
#include <string_view>

namespace ui
{
    // Internal editing model. Positions are UTF-8 code-point offsets, never raw byte offsets.
    class TextEditState
    {
    public:
        TextEditState() = default;
        explicit TextEditState(std::string text);

        const std::string &text() const noexcept;
        void setText(std::string text);

        std::size_t caret() const noexcept;
        std::size_t anchor() const noexcept;
        std::size_t textLength() const noexcept;

        std::size_t selectionStart() const noexcept;
        std::size_t selectionEnd() const noexcept;
        bool hasSelection() const noexcept;

        void setCaret(std::size_t position) noexcept;
        void extendSelectionTo(std::size_t position) noexcept;
        void collapseSelectionToCaret() noexcept;
        void selectAll() noexcept;

        void moveLeft(bool extendSelection) noexcept;
        void moveRight(bool extendSelection) noexcept;
        void moveHome(bool extendSelection) noexcept;
        void moveEnd(bool extendSelection) noexcept;

        void insertText(std::string_view text);
        void backspace();
        void deleteForward();

    private:
        static std::size_t codePointCount(std::string_view text) noexcept;
        static std::size_t byteOffsetForCodePoint(std::string_view text, std::size_t codePointIndex) noexcept;
        static std::size_t previousCodePoint(std::string_view text, std::size_t codePointIndex) noexcept;
        static std::size_t nextCodePoint(std::string_view text, std::size_t codePointIndex) noexcept;

        void eraseSelection();
        void eraseCodePointBeforeCaret();
        void eraseCodePointAtCaret();
        void normalizeSelection() noexcept;

        std::string text_;
        std::size_t caret_ = 0;
        std::size_t anchor_ = 0;
    };
}
