#include "ui_framework/components/text_input.hpp"

#include "event_dispatcher.hpp"
#include "node_tree.hpp"

#include <cassert>
#include <memory>
#include <string>

namespace
{
    ui::TextInput *attachTextInput(ui::NodeTree &tree)
    {
        auto input = std::make_unique<ui::TextInput>();
        ui::TextInput *result = input.get();
        tree.attachRoot(0, std::move(input));
        return result;
    }

    void focus(ui::NodeTree &tree, ui::TextInput *input)
    {
        ui::FocusGainedEvent event;
        ui::EventDispatcher::dispatch(tree, input, event, false, false);
    }

    void testEnterPublishesSubmittedWithoutMutatingText()
    {
        ui::NodeTree tree;
        ui::TextInput *input = attachTextInput(tree);
        focus(tree, input);
        input->setText("message");

        int submitted = 0;
        std::string observedText;
        input->on<ui::TextInputSubmittedEvent>(
            [&](ui::TextInputSubmittedEvent &, ui::Node &node)
            {
                ++submitted;
                observedText = static_cast<ui::TextInput &>(node).getText();
            });

        ui::KeyDownEvent enter;
        enter.key = ui::KeyCode::ENTER;
        ui::EventDispatcher::dispatch(tree, input, enter, false, false);

        assert(submitted == 1);
        assert(observedText == "message");
        assert(input->getText() == "message");
        assert(enter.propagationStopped);
    }

    void testEnterWithSelectionDoesNotDiscardText()
    {
        ui::NodeTree tree;
        ui::TextInput *input = attachTextInput(tree);
        focus(tree, input);
        input->setText("message");
        input->selectAll();

        int submitted = 0;
        input->on<ui::TextInputSubmittedEvent>(
            [&](ui::TextInputSubmittedEvent &, ui::Node &)
            {
                ++submitted;
            });

        ui::KeyDownEvent enter;
        enter.key = ui::KeyCode::ENTER;
        ui::EventDispatcher::dispatch(tree, input, enter, false, false);

        assert(submitted == 1);
        assert(input->getText() == "message");
        assert(input->hasSelection());
    }

    void testEscapeRemainsClientHandled()
    {
        ui::NodeTree tree;
        ui::TextInput *input = attachTextInput(tree);
        focus(tree, input);
        input->setText("message");
        input->selectAll();

        ui::KeyDownEvent escape;
        escape.key = ui::KeyCode::ESCAPE;
        ui::EventDispatcher::dispatch(tree, input, escape, false, false);

        assert(input->getText() == "message");
        assert(input->hasSelection());
        assert(!escape.propagationStopped);
    }
}

int main()
{
    testEnterPublishesSubmittedWithoutMutatingText();
    testEnterWithSelectionDoesNotDiscardText();
    testEscapeRemainsClientHandled();
    return 0;
}
