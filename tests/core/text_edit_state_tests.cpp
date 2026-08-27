#include "text_edit_state.hpp"

#include <cassert>
#include <string>

namespace
{
    void testInitialCaret()
    {
        ui::TextEditState state("hello");
        assert(state.text() == "hello");
        assert(state.caret() == 5);
        assert(!state.hasSelection());
    }

    void testInsertionAndCaret()
    {
        ui::TextEditState state("ac");
        state.setCaret(1);
        state.insertText("b");
        assert(state.text() == "abc");
        assert(state.caret() == 2);
    }

    void testSelectionReplacement()
    {
        ui::TextEditState state("hello world");
        state.setCaret(6);
        state.moveRight(true);
        state.moveRight(true);
        state.moveRight(true);
        state.moveRight(true);
        state.moveRight(true);
        assert(state.selectionStart() == 6);
        assert(state.selectionEnd() == 11);
        state.insertText("there");
        assert(state.text() == "hello there");
        assert(!state.hasSelection());
        assert(state.caret() == 11);
    }

    void testBackspaceAndDelete()
    {
        ui::TextEditState state("abc");
        state.setCaret(2);
        state.backspace();
        assert(state.text() == "ac");
        assert(state.caret() == 1);

        state.deleteForward();
        assert(state.text() == "a");
        assert(state.caret() == 1);
    }

    void testSelectionCommands()
    {
        ui::TextEditState state("hello");
        state.selectAll();
        assert(state.selectionStart() == 0);
        assert(state.selectionEnd() == 5);

        state.moveLeft(false);
        assert(state.caret() == 0);
        assert(!state.hasSelection());

        state.moveEnd(false);
        state.moveLeft(true);
        assert(state.selectionStart() == 4);
        assert(state.selectionEnd() == 5);
    }

    void testUtf8CaretAndDeletion()
    {
        ui::TextEditState state("a€b");
        assert(state.textLength() == 3);
        assert(state.caret() == 3);

        state.backspace();
        assert(state.text() == "a€");
        assert(state.caret() == 2);

        state.backspace();
        assert(state.text() == "a");
        assert(state.caret() == 1);
    }

    void testFourByteUtf8CaretAndDeletion()
    {
        ui::TextEditState state("A😀B");
        assert(state.textLength() == 3);
        assert(state.caret() == 3);

        state.moveLeft(false);
        assert(state.caret() == 2);
        state.moveLeft(false);
        assert(state.caret() == 1);

        state.moveRight(false);
        assert(state.caret() == 2);

        state.backspace();
        assert(state.text() == "AB");
        assert(state.caret() == 1);

        state.setText("A😀B");
        state.setCaret(1);
        state.deleteForward();
        assert(state.text() == "AB");
        assert(state.caret() == 1);
    }

    void testFourByteUtf8SelectionReplacement()
    {
        ui::TextEditState state("A😀B");
        state.setCaret(1);
        state.moveRight(true);
        assert(state.selectionStart() == 1);
        assert(state.selectionEnd() == 2);

        state.insertText("x");
        assert(state.text() == "AxB");
        assert(state.caret() == 2);
        assert(!state.hasSelection());
    }
}

int main()
{
    testInitialCaret();
    testInsertionAndCaret();
    testSelectionReplacement();
    testBackspaceAndDelete();
    testSelectionCommands();
    testUtf8CaretAndDeletion();
    testFourByteUtf8CaretAndDeletion();
    testFourByteUtf8SelectionReplacement();
    return 0;
}
