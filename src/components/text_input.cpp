#include "ui_framework/components/text_input.hpp"

#include <algorithm>
#include <utility>

#include <SDL3/SDL.h>

#include "../core/text_content.hpp"
#include "../core/text_edit_state.hpp"

namespace ui
{
    TextInput::TextInput()
        : editState_(std::make_unique<TextEditState>()),
          text_(std::make_unique<TextContent>())
    {
        setFocusable(true);
        setCapturable(true);
        text_->setWrapMode(WrapMode::NO_WRAP);

        on<FocusGainedEvent>([this](FocusGainedEvent &event, Node &node) { handleFocusGained(event, node); });
        on<FocusLostEvent>([this](FocusLostEvent &event, Node &node) { handleFocusLost(event, node); });
        on<MouseDownEvent>([this](MouseDownEvent &event, Node &node) { handleMouseDown(event, node); });
        on<MouseDragEvent>([this](MouseDragEvent &event, Node &node) { handleMouseDrag(event, node); });
        on<MouseDragEndEvent>([this](MouseDragEndEvent &event, Node &node) { handleMouseDragEnd(event, node); });
        on<KeyDownEvent>([this](KeyDownEvent &event, Node &node) { handleKeyDown(event, node); });
        on<TextInputEvent>([this](TextInputEvent &event, Node &node) { handleTextInput(event, node); });
        on<TextEditingEvent>([this](TextEditingEvent &event, Node &node) { handleTextEditing(event, node); });
    }

    TextInput::~TextInput() = default;

    void TextInput::setText(std::string text)
    {
        if (editState_->text() == text)
            return;
        editState_->setText(std::move(text));
        syncTextContent();
        markTextChanged();
    }

    const std::string &TextInput::getText() const noexcept { return editState_->text(); }

    void TextInput::setPlaceholder(std::string text)
    {
        placeholder_ = std::move(text);
        syncTextContent();
    }

    const std::string &TextInput::getPlaceholder() const noexcept { return placeholder_; }

    void TextInput::setFont(TTF_Font *font) noexcept
    {
        text_->setFont(font);
        invalidateLayout();
    }

    TTF_Font *TextInput::getFont() const noexcept
    {
        return text_->getFont();
    }

    std::size_t TextInput::getCaretPosition() const noexcept { return editState_->caret(); }
    std::size_t TextInput::getSelectionStart() const noexcept { return editState_->selectionStart(); }
    std::size_t TextInput::getSelectionEnd() const noexcept { return editState_->selectionEnd(); }
    bool TextInput::hasSelection() const noexcept { return editState_->hasSelection(); }
    void TextInput::setCaretPosition(std::size_t position) noexcept { editState_->setCaret(position); }
    void TextInput::selectAll() noexcept { editState_->selectAll(); }
    void TextInput::clearSelection() noexcept { editState_->collapseSelectionToCaret(); }

    void TextInput::insertText(std::string_view text)
    {
        if (text.empty())
            return;
        clearComposition();
        editState_->insertText(text);
        syncTextContent();
        markTextChanged();
    }

    void TextInput::backspace()
    {
        const std::string before = editState_->text();
        editState_->backspace();
        if (editState_->text() == before)
            return;
        clearComposition();
        syncTextContent();
        markTextChanged();
    }

    void TextInput::deleteForward()
    {
        const std::string before = editState_->text();
        editState_->deleteForward();
        if (editState_->text() == before)
            return;
        clearComposition();
        syncTextContent();
        markTextChanged();
    }

    void TextInput::moveCaretLeft(bool extendSelection) noexcept { editState_->moveLeft(extendSelection); }
    void TextInput::moveCaretRight(bool extendSelection) noexcept { editState_->moveRight(extendSelection); }
    void TextInput::moveCaretHome(bool extendSelection) noexcept { editState_->moveHome(extendSelection); }
    void TextInput::moveCaretEnd(bool extendSelection) noexcept { editState_->moveEnd(extendSelection); }

    LayoutSize TextInput::measureContent(const LayoutSize &availableContent) const
    {
        return text_->measure(availableContent.width);
    }

    void TextInput::arrangeContent(const LayoutPosition &contentPosition, const LayoutSize &contentSize)
    {
        text_->arrange(contentPosition, contentSize);
    }

    void TextInput::draw(SDL_Renderer *renderer)
    {
        if (!renderer)
            return;

        const LayoutPosition textPosition = text_->getArrangedPosition();
        const LayoutSize actualSize = getActualSize();

        Uint8 oldR = 0;
        Uint8 oldG = 0;
        Uint8 oldB = 0;
        Uint8 oldA = 255;
        const bool haveOldColor = SDL_GetRenderDrawColor(renderer, &oldR, &oldG, &oldB, &oldA);

        if (focused_ && editState_->hasSelection())
        {
            const float start = text_->caretOffset(editState_->selectionStart());
            const float end = text_->caretOffset(editState_->selectionEnd());
            const float width = std::max(0.0f, end - start);
            const SDL_FRect selectionRect{textPosition.x + start, textPosition.y, width, actualSize.height};

            SDL_SetRenderDrawColor(renderer, 70, 110, 190, 110);
            SDL_RenderFillRect(renderer, &selectionRect);
        }

        text_->draw(renderer);

        if (focused_)
        {
            const float caretX = textPosition.x + text_->caretOffset(editState_->caret());
            const SDL_FRect caretRect{caretX, textPosition.y, 1.0f, actualSize.height};

            SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
            SDL_RenderFillRect(renderer, &caretRect);
        }

        if (haveOldColor)
            SDL_SetRenderDrawColor(renderer, oldR, oldG, oldB, oldA);
    }

    void TextInput::handleFocusGained(FocusGainedEvent &, Node &)
    {
        focused_ = true;
        syncTextContent();
    }

    void TextInput::handleFocusLost(FocusLostEvent &, Node &)
    {
        focused_ = false;
        pointerSelecting_ = false;
        clearComposition();
        syncTextContent();
    }

    void TextInput::handleMouseDown(MouseDownEvent &event, Node &)
    {
        if (event.button != MouseButton::Left)
            return;

        clearComposition();
        setCaretFromPointer(event.position.x, false);
        pointerSelectionAnchor_ = editState_->caret();
        pointerSelecting_ = true;
        event.stopPropagation();
    }

    void TextInput::handleMouseDrag(MouseDragEvent &event, Node &)
    {
        if (!focused_ || !pointerSelecting_)
            return;

        setCaretFromPointer(event.position.x, true);
        event.stopPropagation();
    }

    void TextInput::handleMouseDragEnd(MouseDragEndEvent &event, Node &)
    {
        if (!focused_ || !pointerSelecting_)
            return;

        setCaretFromPointer(event.position.x, true);
        pointerSelecting_ = false;
        event.stopPropagation();
    }

    void TextInput::setCaretFromPointer(float x, bool extendSelection)
    {
        const LayoutPosition textPosition = text_->getArrangedPosition();
        const float localX = x - textPosition.x;
        const std::size_t position = text_->textPositionAt(localX);

        if (extendSelection)
        {
            editState_->setCaret(pointerSelectionAnchor_);
            editState_->extendSelectionTo(position);
        }
        else
        {
            editState_->setCaret(position);
        }
    }

    void TextInput::handleKeyDown(KeyDownEvent &event, Node &)
    {
        if (!focused_)
            return;

        if (event.modifiers.ctrl && event.key == KeyCode::A)
        {
            clearComposition();
            selectAll();
            event.stopPropagation();
            return;
        }

        if (event.key == KeyCode::ENTER)
        {
            clearComposition();
            TextInputSubmittedEvent submitted;
            emit(submitted);
            event.stopPropagation();
            return;
        }

        bool handled = true;
        switch (event.key)
        {
        case KeyCode::LEFT: moveCaretLeft(event.modifiers.shift); break;
        case KeyCode::RIGHT: moveCaretRight(event.modifiers.shift); break;
        case KeyCode::HOME: moveCaretHome(event.modifiers.shift); break;
        case KeyCode::END: moveCaretEnd(event.modifiers.shift); break;
        case KeyCode::BACKSPACE: backspace(); break;
        case KeyCode::DELETE: deleteForward(); break;
        default: handled = false; break;
        }

        if (handled)
            event.stopPropagation();
    }

    void TextInput::handleTextInput(TextInputEvent &event, Node &)
    {
        if (!focused_ || event.text.empty())
            return;
        clearComposition();
        editState_->insertText(event.text);
        syncTextContent();
        markTextChanged();
        event.stopPropagation();
    }

    void TextInput::handleTextEditing(TextEditingEvent &event, Node &)
    {
        if (!focused_)
            return;

        composition_ = event.composition;
        compositionCursor_ = std::max(0, event.cursor);
        compositionSelectionLength_ = std::max(0, event.selectionLength);
    }

    void TextInput::markTextChanged()
    {
        invalidateLayout();
        TextChangedEvent event;
        emit(event);
    }

    void TextInput::emitSubmitted()
    {
        TextInputSubmittedEvent event;
        emit(event);
    }

    void TextInput::clearComposition() noexcept
    {
        composition_.clear();
        compositionCursor_ = 0;
        compositionSelectionLength_ = 0;
    }

    void TextInput::syncTextContent()
    {
        if (editState_->text().empty() && !focused_ && !placeholder_.empty())
            text_->setText(placeholder_);
        else
            text_->setText(editState_->text());
    }
}