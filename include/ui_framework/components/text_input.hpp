#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>

#include <SDL3_ttf/SDL_ttf.h>

#include "ui_framework/events.hpp"
#include "ui_framework/node.hpp"

namespace ui
{
    class TextContent;
    class TextEditState;

    struct TextChangedEvent : UIEvent
    {
    };

    struct TextInputSubmittedEvent : UIEvent
    {
    };

    class TextInput : public Node
    {
    public:
        TextInput();
        ~TextInput() override;
        TextInput(const TextInput &) = delete;
        TextInput &operator=(const TextInput &) = delete;

        void setText(std::string text);
        const std::string &getText() const noexcept;
        void setPlaceholder(std::string text);
        const std::string &getPlaceholder() const noexcept;
        void setFont(TTF_Font *font) noexcept;
        TTF_Font *getFont() const noexcept;

        std::size_t getCaretPosition() const noexcept;
        std::size_t getSelectionStart() const noexcept;
        std::size_t getSelectionEnd() const noexcept;
        bool hasSelection() const noexcept;
        void setCaretPosition(std::size_t position) noexcept;
        void selectAll() noexcept;
        void clearSelection() noexcept;

        void insertText(std::string_view text);
        void backspace();
        void deleteForward();
        void moveCaretLeft(bool extendSelection = false) noexcept;
        void moveCaretRight(bool extendSelection = false) noexcept;
        void moveCaretHome(bool extendSelection = false) noexcept;
        void moveCaretEnd(bool extendSelection = false) noexcept;

    protected:
        LayoutSize measureContent(const LayoutSize &availableContent) const override;
        void arrangeContent(const LayoutPosition &contentPosition, const LayoutSize &contentSize) override;
        void draw(SDL_Renderer *renderer) override;

    private:
        void handleFocusGained(FocusGainedEvent &event, Node &node);
        void handleFocusLost(FocusLostEvent &event, Node &node);
        void handleMouseDown(MouseDownEvent &event, Node &node);
        void handleMouseDrag(MouseDragEvent &event, Node &node);
        void handleMouseDragEnd(MouseDragEndEvent &event, Node &node);
        void handleKeyDown(KeyDownEvent &event, Node &node);
        void handleTextInput(TextInputEvent &event, Node &node);
        void handleTextEditing(TextEditingEvent &event, Node &node);
        void markTextChanged();
        void emitSubmitted();
        void syncTextContent();
        void clearComposition() noexcept;
        void setCaretFromPointer(float x, bool extendSelection);

        std::unique_ptr<TextEditState> editState_;
        std::unique_ptr<TextContent> text_;
        std::string placeholder_;
        std::string composition_;
        int compositionCursor_ = 0;
        int compositionSelectionLength_ = 0;
        std::size_t pointerSelectionAnchor_ = 0;
        bool pointerSelecting_ = false;
        bool focused_ = false;
    };
}
