#include "ui_framework/ui_manager.hpp"
#include "ui_framework/node.hpp"
#include "ui_framework/panel_node.hpp"
#include "node_tree.hpp"
#include "input_system.hpp"
#include "modal_system.hpp"

#include <SDL3/SDL.h>

#include <cstdlib>
#include <iostream>
#include <memory>

namespace
{
    struct TestFailure
    {
        const char *message;
    };

    void expect(bool condition, const char *message)
    {
        if (!condition)
            throw TestFailure{message};
    }

    SDL_Event keyDown(SDL_Keycode key)
    {
        SDL_Event event{};
        event.type = SDL_EVENT_KEY_DOWN;
        event.key.key = key;
        return event;
    }

    SDL_Event mouseDown(float x, float y)
    {
        SDL_Event event{};
        event.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
        event.button.x = x;
        event.button.y = y;
        event.button.button = SDL_BUTTON_LEFT;
        return event;
    }

    std::unique_ptr<ui::Node> makeModal(float width = 100.0f, float height = 100.0f)
    {
        auto node = std::make_unique<ui::Node>();
        node->setSize(ui::LayoutSizeValue::fixed(width, height));
        node->setFocusable(true);
        return node;
    }

    void prepare(ui::UIManager &manager, ui::Node &node)
    {
        manager.invalidateLayout(node);
        manager.render(nullptr);
    }

    void test_modal_owns_interaction_without_pausing_lower_modals()
    {
        ui::UIManager manager;
        int upperClicks = 0;
        int lowerClicks = 0;

        auto lower = std::make_unique<ui::PanelNode>();
        lower->setSize(ui::LayoutSizeValue::fixed(100.0f, 100.0f));
        lower->setFocusable(true);
        ui::Node *lowerNode = manager.addOverlay(std::move(lower));

        auto upper = std::make_unique<ui::Node>();
        upper->setSize(ui::LayoutSizeValue::fixed(100.0f, 100.0f));
        upper->setFocusable(true);
        ui::Node *upperNode = manager.addOverlay(std::move(upper));

        lowerNode->on<ui::MouseDownEvent>([&](ui::MouseDownEvent &, ui::Node &)
                                          { ++lowerClicks; });
        upperNode->on<ui::MouseDownEvent>([&](ui::MouseDownEvent &, ui::Node &)
                                          { ++upperClicks; });

        expect(manager.showModal(*lowerNode), "first modal must open");
        expect(manager.showModal(*upperNode), "second modal must open above first");
        expect(!manager.showModal(*upperNode), "the same node must not be pushed twice");
        expect(manager.getActiveModal() == upperNode, "top modal must own interaction");

        prepare(manager, *upperNode);
        manager.processEvent(mouseDown(10.0f, 10.0f));
        expect(upperClicks == 1, "top modal must receive pointer input");
        expect(lowerClicks == 0, "lower modal must not receive pointer input");

        manager.advanceTime(1.0f / 60.0f);
    }

    void test_modal_focus_and_tab_trap()
    {
        ui::UIManager manager;
        auto modal = std::make_unique<ui::PanelNode>();
        modal->setSize(ui::LayoutSizeValue::fixed(100.0f, 100.0f));

        auto first = std::make_unique<ui::Node>();
        first->setFocusable(true);
        first->setSize(ui::LayoutSizeValue::fixed(20.0f, 20.0f));
        ui::Node *firstPtr = first.get();
        modal->addChild(std::move(first), 0);

        auto second = std::make_unique<ui::Node>();
        second->setFocusable(true);
        second->setSize(ui::LayoutSizeValue::fixed(20.0f, 20.0f));
        ui::Node *secondPtr = second.get();
        modal->addChild(std::move(second), 1);

        ui::Node *modalPtr = manager.addOverlay(std::move(modal));
        prepare(manager, *modalPtr);

        int firstEscape = 0;
        int secondEscape = 0;
        firstPtr->on<ui::KeyDownEvent>([&](ui::KeyDownEvent &event, ui::Node &)
                                       {
            if (event.key == ui::KeyCode::ESCAPE)
            {
                ++firstEscape;
                event.stopPropagation();
            } });
        secondPtr->on<ui::KeyDownEvent>([&](ui::KeyDownEvent &event, ui::Node &)
                                        {
            if (event.key == ui::KeyCode::ESCAPE)
            {
                ++secondEscape;
                event.stopPropagation();
            } });

        expect(manager.showModal(*modalPtr), "modal must open");
        manager.processEvent(keyDown(SDLK_ESCAPE));
        expect(firstEscape == 1 && secondEscape == 0,
               "opening a non-focusable modal must focus its first focusable child");

        manager.processEvent(keyDown(SDLK_TAB));
        manager.processEvent(keyDown(SDLK_ESCAPE));
        expect(secondEscape == 1,
               "TAB must move focus to the next focusable child inside the modal");

        manager.processEvent(keyDown(SDLK_TAB));
        manager.processEvent(keyDown(SDLK_ESCAPE));
        expect(firstEscape == 2,
               "TAB focus traversal must wrap back to the first focusable child");
        expect(manager.getActiveModal() == modalPtr,
               "a focused child consuming Escape must keep the modal open");
    }

    void test_escape_can_be_consumed_or_disabled()
    {
        ui::UIManager manager;
        auto modal = makeModal();
        ui::Node *modalPtr = manager.addOverlay(std::move(modal));

        expect(manager.showModal(*modalPtr), "modal must open");
        manager.processEvent(keyDown(SDLK_ESCAPE));
        expect(manager.getActiveModal() == nullptr, "unconsumed Escape must close modal");

        ui::ModalOptions options;
        options.closeOnEscape = false;
        expect(manager.showModal(*modalPtr, options), "modal must reopen with Escape disabled");
        manager.processEvent(keyDown(SDLK_ESCAPE));
        expect(manager.getActiveModal() == modalPtr,
               "closeOnEscape=false must prevent Escape from closing the modal");
    }

    void test_outside_click_policy_is_independent_of_backdrop()
    {
        ui::UIManager manager;
        auto modal = makeModal(20.0f, 20.0f);
        ui::Node *modalPtr = manager.addOverlay(std::move(modal));

        ui::ModalOptions options;
        options.showBackdrop = false;
        options.outsideClick = ui::OutsideClickBehavior::Close;

        expect(manager.showModal(*modalPtr, options), "modal with custom options must open");
        manager.processEvent(mouseDown(80.0f, 80.0f));
        expect(manager.getActiveModal() == nullptr,
               "outside click must close modal even without a backdrop");
    }

    void test_outside_click_consume_blocks_background_input()
    {
        ui::UIManager manager;
        int backgroundClicks = 0;

        auto background = std::make_unique<ui::Node>();
        background->setSize(ui::LayoutSizeValue::fixed(100.0f, 100.0f));
        background->on<ui::MouseDownEvent>([&](ui::MouseDownEvent &, ui::Node &)
                                           { ++backgroundClicks; });
        manager.addRoot(std::move(background));

        auto modal = makeModal(20.0f, 20.0f);
        ui::Node *modalPtr = manager.addOverlay(std::move(modal));

        ui::ModalOptions options;
        options.showBackdrop = false;
        options.outsideClick = ui::OutsideClickBehavior::Consume;

        expect(manager.showModal(*modalPtr, options), "modal must open with consume policy");
        manager.processEvent(mouseDown(80.0f, 80.0f));

        expect(manager.getActiveModal() == modalPtr,
               "consume policy must keep the modal open");
        expect(backgroundClicks == 0,
               "consume policy must prevent outside input from reaching background content");
    }

    void test_keyboard_input_never_reaches_lower_modal()
    {
        ui::UIManager manager;
        int lowerKeys = 0;
        int upperKeys = 0;

        auto lower = makeModal();
        lower->on<ui::KeyDownEvent>([&](ui::KeyDownEvent &, ui::Node &)
                                    { ++lowerKeys; });
        ui::Node *lowerPtr = manager.addOverlay(std::move(lower));

        auto upper = makeModal();
        upper->on<ui::KeyDownEvent>([&](ui::KeyDownEvent &, ui::Node &)
                                    { ++upperKeys; });
        ui::Node *upperPtr = manager.addOverlay(std::move(upper));

        expect(manager.showModal(*lowerPtr), "lower modal must open");
        expect(manager.showModal(*upperPtr), "upper modal must open");

        manager.processEvent(keyDown(SDLK_A));

        expect(upperKeys == 1, "top modal must receive keyboard input");
        expect(lowerKeys == 0, "lower modal must never receive keyboard input while blocked");
    }

    void test_nested_close_restores_previous_modal_focus_scope()
    {
        ui::NodeTree tree;
        ui::InputSystem input;
        ui::ModalSystem modalSystem;

        auto lower = std::make_unique<ui::PanelNode>();
        lower->setSize(ui::LayoutSizeValue::fixed(100.0f, 100.0f));
        auto lowerFocus = std::make_unique<ui::Node>();
        lowerFocus->setFocusable(true);
        lowerFocus->setSize(ui::LayoutSizeValue::fixed(20.0f, 20.0f));
        ui::Node *lowerFocusPtr = lowerFocus.get();
        lower->addChild(std::move(lowerFocus), 0);
        ui::Node *lowerPtr = tree.attachOverlay(0, std::move(lower));

        auto upper = makeModal();
        ui::Node *upperPtr = tree.attachOverlay(1, std::move(upper));

        expect(input.focus(tree, *lowerFocusPtr), "lower modal focus target must be focusable");
        expect(modalSystem.showModal(tree, input, *lowerPtr), "lower modal must open");
        expect(input.focusedNode() == lowerFocusPtr, "lower modal child must own the initial focus scope");

        expect(modalSystem.showModal(tree, input, *upperPtr), "upper modal must open");
        expect(input.focusedNode() == upperPtr, "upper modal must become the focused scope");

        expect(modalSystem.closeModal(tree, input), "closing top modal must succeed");
        expect(modalSystem.topModalNode(tree) == lowerPtr,
               "closing top modal must reveal the previous modal");
        expect(input.focusedNode() == lowerFocusPtr,
               "closing top modal must restore the previous modal focus scope");
    }

    void test_removing_lower_modal_closes_entire_modal_branch()
    {
        ui::UIManager manager;
        auto lower = makeModal();
        ui::Node *lowerPtr = manager.addOverlay(std::move(lower));

        auto upper = makeModal();
        ui::Node *upperPtr = manager.addOverlay(std::move(upper));

        expect(manager.showModal(*lowerPtr), "lower modal must open");
        expect(manager.showModal(*upperPtr), "upper modal must open");

        manager.removeOverlay(lowerPtr);
        manager.advanceTime(0.0f);
        manager.render(nullptr);

        expect(manager.getActiveModal() == nullptr,
               "removing the base modal must invalidate the entire modal branch");
    }

    void test_capture_is_cancelled_when_new_modal_opens()
    {
        ui::NodeTree tree;
        ui::InputSystem input;
        ui::ModalSystem modalSystem;

        auto first = makeModal();
        first->setCapturable(true);
        ui::Node *firstPtr = tree.attachOverlay(0, std::move(first));

        auto second = makeModal();
        ui::Node *secondPtr = tree.attachOverlay(1, std::move(second));

        expect(input.capture(tree, *firstPtr), "first node must be capturable");
        expect(input.capturedNode() == firstPtr, "capture must be established before opening modal");

        expect(modalSystem.showModal(tree, input, *secondPtr),
               "second node must open as modal");
        expect(input.capturedNode() == nullptr,
               "opening a modal must cancel any previous pointer capture");
    }

    void test_backdrop_lifecycle_follows_modal_stack()
    {
        ui::UIManager manager;
        manager.setBackdropFadeDuration(0.1f);

        auto modal = makeModal();
        ui::Node *modalPtr = manager.addOverlay(std::move(modal));

        expect(manager.showModal(*modalPtr), "modal must open");

        manager.advanceTime(0.1f);
        manager.closeModal();
        manager.advanceTime(0.1f);
        manager.render(nullptr);

        expect(manager.getActiveModal() == nullptr,
               "modal must remain closed after backdrop fade-out");
    }
}

int main()
{
    try
    {
        test_modal_owns_interaction_without_pausing_lower_modals();
        test_modal_focus_and_tab_trap();
        test_escape_can_be_consumed_or_disabled();
        test_outside_click_policy_is_independent_of_backdrop();
        test_outside_click_consume_blocks_background_input();
        test_keyboard_input_never_reaches_lower_modal();
        test_nested_close_restores_previous_modal_focus_scope();
        test_removing_lower_modal_closes_entire_modal_branch();
        test_capture_is_cancelled_when_new_modal_opens();
        test_backdrop_lifecycle_follows_modal_stack();
    }
    catch (const TestFailure &failure)
    {
        std::cerr << "ModalSystem regression tests failed: " << failure.message << '\n';
        return EXIT_FAILURE;
    }

    std::cout << "ModalSystem regression tests passed\n";
    return EXIT_SUCCESS;
}
