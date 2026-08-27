#include "text_edit_state.hpp"

#include <algorithm>
#include <utility>

namespace ui
{
    TextEditState::TextEditState(std::string text)
        : text_(std::move(text))
    {
        caret_ = textLength();
        anchor_ = caret_;
    }

    const std::string &TextEditState::text() const noexcept { return text_; }

    void TextEditState::setText(std::string text)
    {
        text_ = std::move(text);
        caret_ = std::min(caret_, textLength());
        anchor_ = std::min(anchor_, textLength());
        normalizeSelection();
    }

    std::size_t TextEditState::caret() const noexcept { return caret_; }
    std::size_t TextEditState::anchor() const noexcept { return anchor_; }
    std::size_t TextEditState::textLength() const noexcept { return codePointCount(text_); }

    std::size_t TextEditState::selectionStart() const noexcept { return std::min(caret_, anchor_); }
    std::size_t TextEditState::selectionEnd() const noexcept { return std::max(caret_, anchor_); }
    bool TextEditState::hasSelection() const noexcept { return caret_ != anchor_; }

    void TextEditState::setCaret(std::size_t position) noexcept
    {
        caret_ = std::min(position, textLength());
        anchor_ = caret_;
    }

    void TextEditState::extendSelectionTo(std::size_t position) noexcept
    {
        caret_ = std::min(position, textLength());
    }

    void TextEditState::collapseSelectionToCaret() noexcept { anchor_ = caret_; }

    void TextEditState::selectAll() noexcept
    {
        anchor_ = 0;
        caret_ = textLength();
    }

    void TextEditState::moveLeft(bool extendSelection) noexcept
    {
        if (!extendSelection && hasSelection())
        {
            caret_ = selectionStart();
            anchor_ = caret_;
            return;
        }
        caret_ = previousCodePoint(text_, caret_);
        if (!extendSelection) anchor_ = caret_;
    }

    void TextEditState::moveRight(bool extendSelection) noexcept
    {
        if (!extendSelection && hasSelection())
        {
            caret_ = selectionEnd();
            anchor_ = caret_;
            return;
        }
        caret_ = nextCodePoint(text_, caret_);
        if (!extendSelection) anchor_ = caret_;
    }

    void TextEditState::moveHome(bool extendSelection) noexcept
    {
        caret_ = 0;
        if (!extendSelection) anchor_ = caret_;
    }

    void TextEditState::moveEnd(bool extendSelection) noexcept
    {
        caret_ = textLength();
        if (!extendSelection) anchor_ = caret_;
    }

    void TextEditState::insertText(std::string_view text)
    {
        if (hasSelection()) eraseSelection();
        const auto byteOffset = byteOffsetForCodePoint(text_, caret_);
        text_.insert(byteOffset, text.data(), text.size());
        caret_ += codePointCount(text);
        anchor_ = caret_;
    }

    void TextEditState::backspace()
    {
        if (hasSelection())
        {
            eraseSelection();
            return;
        }
        eraseCodePointBeforeCaret();
    }

    void TextEditState::deleteForward()
    {
        if (hasSelection())
        {
            eraseSelection();
            return;
        }
        eraseCodePointAtCaret();
    }

    std::size_t TextEditState::codePointCount(std::string_view text) noexcept
    {
        std::size_t count = 0;
        for (unsigned char c : text)
            if ((c & 0xC0u) != 0x80u) ++count;
        return count;
    }

    std::size_t TextEditState::byteOffsetForCodePoint(std::string_view text, std::size_t index) noexcept
    {
        if (index == 0) return 0;
        std::size_t count = 0;
        for (std::size_t i = 0; i < text.size(); ++i)
        {
            if ((static_cast<unsigned char>(text[i]) & 0xC0u) != 0x80u && count++ == index)
                return i;
        }
        return text.size();
    }

    std::size_t TextEditState::previousCodePoint(std::string_view text, std::size_t index) noexcept
    {
        if (index == 0) return 0;
        return index - 1;
    }

    std::size_t TextEditState::nextCodePoint(std::string_view text, std::size_t index) noexcept
    {
        const auto length = codePointCount(text);
        return std::min(index + 1, length);
    }

    void TextEditState::eraseSelection()
    {
        const auto start = selectionStart();
        const auto end = selectionEnd();
        const auto startByte = byteOffsetForCodePoint(text_, start);
        const auto endByte = byteOffsetForCodePoint(text_, end);
        text_.erase(startByte, endByte - startByte);
        caret_ = start;
        anchor_ = caret_;
    }

    void TextEditState::eraseCodePointBeforeCaret()
    {
        if (caret_ == 0) return;
        const auto start = byteOffsetForCodePoint(text_, caret_ - 1);
        const auto end = byteOffsetForCodePoint(text_, caret_);
        text_.erase(start, end - start);
        --caret_;
        anchor_ = caret_;
    }

    void TextEditState::eraseCodePointAtCaret()
    {
        if (caret_ >= textLength()) return;
        const auto start = byteOffsetForCodePoint(text_, caret_);
        const auto end = byteOffsetForCodePoint(text_, caret_ + 1);
        text_.erase(start, end - start);
        anchor_ = caret_;
    }

    void TextEditState::normalizeSelection() noexcept
    {
        const auto length = textLength();
        caret_ = std::min(caret_, length);
        anchor_ = std::min(anchor_, length);
    }
}
