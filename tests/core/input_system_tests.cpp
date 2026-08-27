#include "input_system.hpp"
#include "layout_system.hpp"
#include "node_tree.hpp"

#include <SDL3/SDL.h>

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace
{
    struct TestFailure
    {
        std::string message;
    };

    void expect(bool condition, const char *message)
    {
        if (!condition)
            throw TestFailure{message};
    }

    struct Fixture
    {
        ui::NodeTree tree;
        ui::LayoutSystem layout;
        ui::Node *root = nullptr;
        ui::InputSystem input;

        Fixture()
        {
            auto node = std::make_unique<ui::Node>();
            node->setSize(ui::LayoutSizeValue::fixed(100.0f, 100.0f));
            root = tree.attachRoot(0, std::move(node));
            layout.setViewportSize({100.0f, 100.0f});
            layout.requestFullLayout(tree);
            layout.processLayoutQueue(tree);
        }
    };

    SDL_Event mouseMotion(float x, float y)
    {
        SDL_Event event{};
        event.type = SDL_EVENT_MOUSE_MOTION;
        event.motion.x = x;
        event.motion.y = y;
        return event;
    }

    SDL_Event mouseDown(float x, float y, Uint8 button = SDL_BUTTON_LEFT)
    {
        SDL_Event event{};
        event.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
        event.button.x = x;
        event.button.y = y;
        event.button.button = button;
        return event;
    }

    SDL_Event mouseUp(float x, float y, Uint8 button = SDL_BUTTON_LEFT)
    {
        SDL_Event event{};
        event.type = SDL_EVENT_MOUSE_BUTTON_UP;
        event.button.x = x;
        event.button.y = y;
        event.button.button = button;
        return event;
    }

    void test_hover_enter_leave()
    {
        Fixture f;
        int enters = 0;
        int leaves = 0;
        f.root->on<ui::MouseEnterEvent>([&](ui::MouseEnterEvent &, ui::Node &)
                                        { ++enters; });
        f.root->on<ui::MouseLeaveEvent>([&](ui::MouseLeaveEvent &, ui::Node &)
                                        { ++leaves; });

        f.input.processEvent(mouseMotion(10.0f, 10.0f), f.tree, nullptr);
        expect(enters == 1, "hover enter must be dispatched once");
        expect(leaves == 0, "leave must not fire while pointer remains inside");

        f.input.processEvent(mouseMotion(150.0f, 150.0f), f.tree, nullptr);
        expect(leaves == 1, "hover leave must be dispatched once");
    }

    void test_click_sequence_and_automatic_focus_capture()
    {
        Fixture f;
        int downs = 0;
        int ups = 0;
        int clicks = 0;
        f.root->setFocusable(true);
        f.root->setCapturable(true);
        f.root->on<ui::MouseDownEvent>([&](ui::MouseDownEvent &, ui::Node &)
                                       { ++downs; });
        f.root->on<ui::MouseUpEvent>([&](ui::MouseUpEvent &, ui::Node &)
                                     { ++ups; });
        f.root->on<ui::MouseClickEvent>([&](ui::MouseClickEvent &, ui::Node &)
                                        { ++clicks; });

        f.input.processEvent(mouseDown(10.0f, 10.0f), f.tree, nullptr);
        expect(downs == 1, "MouseDown must reach hit target");
        expect(f.input.focusedNode() == f.root, "focusable target must receive focus");
        expect(f.input.capturedNode() == f.root, "capturable target must receive capture");
        expect(f.input.pressedNode() == f.root, "MouseDown must establish pressed target");

        f.input.processEvent(mouseUp(10.0f, 10.0f), f.tree, nullptr);
        expect(ups == 1, "MouseUp must reach captured target");
        expect(clicks == 1, "matching MouseDown/MouseUp must produce Click");
        expect(f.input.capturedNode() == nullptr, "capture must be released after MouseUp");
        expect(f.input.pressedNode() == nullptr, "pressed state must be cleared after MouseUp");
    }

    void test_click_focus_moves_to_second_focusable_target()
    {
        Fixture f;
        f.root->setSize(ui::LayoutSizeValue::fixed(50.0f, 100.0f));
        f.root->setFocusable(true);
        f.root->setCapturable(true);

        auto second = std::make_unique<ui::Node>();
        second->setPosition({50.0f, 0.0f});
        second->setSize(ui::LayoutSizeValue::fixed(50.0f, 100.0f));
        second->setFocusable(true);
        second->setCapturable(true);
        ui::Node *secondPtr = f.tree.attachRoot(1, std::move(second));

        f.layout.setViewportSize({100.0f, 100.0f});
        f.layout.requestFullLayout(f.tree);
        f.layout.processLayoutQueue(f.tree);

        expect(f.input.focus(f.tree, *f.root), "first target must accept focus");
        expect(f.input.focusedNode() == f.root, "first target must be focused initially");

        f.input.processEvent(mouseDown(75.0f, 50.0f), f.tree, nullptr);

        expect(f.input.focusedNode() == secondPtr,
               "MouseDown on another focusable target must move focus to that target");
    }

    void test_mouse_down_callback_can_override_capture()
    {
        Fixture f;
        f.root->setCapturable(true);
        bool callbackCaptureSucceeded = false;
        f.root->on<ui::MouseDownEvent>([&](ui::MouseDownEvent &, ui::Node &node)
                                       { callbackCaptureSucceeded = f.input.capture(f.tree, node); });

        f.input.processEvent(mouseDown(10.0f, 10.0f), f.tree, nullptr);

        expect(callbackCaptureSucceeded, "MouseDown callback must be able to capture explicitly");
        expect(f.input.capturedNode() == f.root,
               "explicit capture must survive automatic capture reconciliation");
    }

    void test_drag_lifecycle_and_threshold()
    {
        Fixture f;
        f.root->setCapturable(true);
        std::vector<std::string> events;
        f.root->on<ui::MouseDownEvent>([&](ui::MouseDownEvent &, ui::Node &)
                                       { events.push_back("down"); });
        f.root->on<ui::MouseMoveEvent>([&](ui::MouseMoveEvent &, ui::Node &)
                                       { events.push_back("move"); });
        f.root->on<ui::MouseDragBeginEvent>([&](ui::MouseDragBeginEvent &, ui::Node &)
                                            { events.push_back("drag_begin"); });
        f.root->on<ui::MouseDragEvent>([&](ui::MouseDragEvent &, ui::Node &)
                                       { events.push_back("drag"); });
        f.root->on<ui::MouseUpEvent>([&](ui::MouseUpEvent &, ui::Node &)
                                     { events.push_back("up"); });
        f.root->on<ui::MouseDragEndEvent>([&](ui::MouseDragEndEvent &, ui::Node &)
                                          { events.push_back("drag_end"); });

        f.input.processEvent(mouseDown(10.0f, 10.0f), f.tree, nullptr);
        expect(!f.input.isDragging(), "MouseDown must not start drag");

        f.input.processEvent(mouseMotion(10.0f + 1.0f, 10.0f), f.tree, nullptr);
        expect(!f.input.isDragging(), "movement below threshold must not start drag");

        f.input.processEvent(mouseMotion(10.0f + 10.0f, 10.0f), f.tree, nullptr);
        expect(f.input.isDragging(), "movement beyond threshold must start drag");

        f.input.processEvent(mouseUp(20.0f, 10.0f), f.tree, nullptr);
        expect(f.input.capturedNode() == nullptr, "MouseUp must release capture after drag");
        expect(f.input.pressedNode() == nullptr, "MouseUp must clear pressed state after drag");
        expect(!f.input.isDragging(), "MouseUp must end drag state");

        const std::vector<std::string> expectedPrefix{"down", "move", "drag_begin", "drag"};
        expect(events.size() >= expectedPrefix.size(), "drag sequence must emit expected initial events");
        for (std::size_t i = 0; i < expectedPrefix.size(); ++i)
            expect(events[i] == expectedPrefix[i], "drag event order must be stable");
        expect(events[events.size() - 2] == "up", "MouseUp must precede DragEnd");
        expect(events.back() == "drag_end", "DragEnd must be the final drag lifecycle event");
    }

    void test_click_is_suppressed_after_drag()
    {
        Fixture f;
        f.root->setCapturable(true);
        int clicks = 0;
        int dragEnds = 0;
        f.root->on<ui::MouseClickEvent>([&](ui::MouseClickEvent &, ui::Node &)
                                        { ++clicks; });
        f.root->on<ui::MouseDragEndEvent>([&](ui::MouseDragEndEvent &, ui::Node &)
                                          { ++dragEnds; });

        f.input.processEvent(mouseDown(10.0f, 10.0f), f.tree, nullptr);
        f.input.processEvent(mouseMotion(30.0f, 10.0f), f.tree, nullptr);
        f.input.processEvent(mouseUp(30.0f, 10.0f), f.tree, nullptr);

        expect(clicks == 0, "dragging must suppress Click");
        expect(dragEnds == 1, "dragging must emit DragEnd exactly once");
    }

    void test_keyboard_routes_to_focused_node()
    {
        Fixture f;
        int downs = 0;
        int ups = 0;
        ui::KeyCode receivedDown = ui::KeyCode::UNKNOWN;
        ui::KeyCode receivedUp = ui::KeyCode::UNKNOWN;
        f.root->setFocusable(true);
        f.root->on<ui::KeyDownEvent>([&](ui::KeyDownEvent &event, ui::Node &)
                                     { ++downs; receivedDown = event.key; });
        f.root->on<ui::KeyUpEvent>([&](ui::KeyUpEvent &event, ui::Node &)
                                   { ++ups; receivedUp = event.key; });

        expect(f.input.focus(f.tree, *f.root), "focus must succeed for focusable node");
        f.input.processEvent(([]
                              { SDL_Event event{}; event.type = SDL_EVENT_KEY_DOWN; event.key.key = SDLK_RETURN; return event; })(),
                             f.tree, nullptr);
        f.input.processEvent(([]
                              { SDL_Event event{}; event.type = SDL_EVENT_KEY_UP; event.key.key = SDLK_RETURN; return event; })(),
                             f.tree, nullptr);

        expect(downs == 1, "KeyDown must reach focused node");
        expect(ups == 1, "KeyUp must reach focused node");
        expect(receivedDown == ui::KeyCode::ENTER, "SDL return must map to ENTER");
        expect(receivedUp == ui::KeyCode::ENTER, "SDL return must map to ENTER");
    }

    void test_capture_survives_pointer_leaving_target()
    {
        Fixture f;
        int moves = 0;
        f.root->setCapturable(true);
        f.root->on<ui::MouseMoveEvent>([&](ui::MouseMoveEvent &, ui::Node &)
                                       { ++moves; });

        expect(f.input.capture(f.tree, *f.root, ui::MousePosition{10.0f, 10.0f}), "capture must succeed");
        f.input.processEvent(mouseMotion(150.0f, 150.0f), f.tree, nullptr);
        expect(moves == 1, "captured node must receive movement outside its bounds");
        expect(f.input.capturedNode() == f.root, "capture must remain active");
    }

    void test_removed_captured_node_is_reconciled()
    {
        Fixture f;
        f.root->setCapturable(true);
        expect(f.input.capture(f.tree, *f.root, ui::MousePosition{10.0f, 10.0f}), "capture must succeed");
        f.tree.removeRoot(f.root);
        f.tree.flushMutationQueue();
        f.input.syncState(f.tree);
        expect(f.input.capturedNode() == nullptr, "removed captured node must not remain tracked");
        expect(f.input.pressedNode() == nullptr, "removed captured node must clear pressed state");
        expect(!f.input.isDragging(), "removed captured node must clear drag state");
    }

    void test_focus_callback_can_request_another_focus()
    {
        Fixture f;
        auto second = std::make_unique<ui::Node>();
        second->setPosition({0.0f, 100.0f});
        second->setSize(ui::LayoutSizeValue::fixed(20.0f, 20.0f));
        ui::Node *secondPtr = f.tree.attachRoot(1, std::move(second));
        secondPtr->setFocusable(true);
        f.layout.requestFullLayout(f.tree);
        f.layout.processLayoutQueue(f.tree);
        f.root->setFocusable(true);
        f.root->on<ui::FocusLostEvent>([&](ui::FocusLostEvent &, ui::Node &)
                                       {
            std::cerr << "    [10 callback] before nested focus\n";
            const bool result = f.input.focus(f.tree, *secondPtr);
            std::cerr << "    [10 callback] nested focus returned: " << result << '\n';
            std::cerr << "    [10 callback] focused == second: " << (f.input.focusedNode() == secondPtr) << '\n'; });

        std::cerr << "    [10] before initial focus\n";
        expect(f.input.focus(f.tree, *f.root), "initial focus must succeed");
        std::cerr << "    [10] before clearFocus\n";
        f.input.clearFocus(f.tree);
        std::cerr << "    [10] after clearFocus\n";
        expect(f.input.focusedNode() == secondPtr, "focus requested during FocusLost must be applied");
        std::cerr << "    [10] final assertion passed\n";
    }
}

int main()
{
    try
    {
        std::cerr << "[1] hover_enter_leave\n";
        test_hover_enter_leave();
        std::cerr << "[1] PASS\n";
        std::cerr << "[2] click_sequence_and_automatic_focus_capture\n";
        test_click_sequence_and_automatic_focus_capture();
        std::cerr << "[2] PASS\n";
        std::cerr << "[3] click_focus_moves_to_second_focusable_target\n";
        test_click_focus_moves_to_second_focusable_target();
        std::cerr << "[3] PASS\n";
        std::cerr << "[4] mouse_down_callback_can_override_capture\n";
        test_mouse_down_callback_can_override_capture();
        std::cerr << "[4] PASS\n";
        std::cerr << "[5] drag_lifecycle_and_threshold\n";
        test_drag_lifecycle_and_threshold();
        std::cerr << "[5] PASS\n";
        std::cerr << "[6] click_is_suppressed_after_drag\n";
        test_click_is_suppressed_after_drag();
        std::cerr << "[6] PASS\n";
        std::cerr << "[7] keyboard_routes_to_focused_node\n";
        test_keyboard_routes_to_focused_node();
        std::cerr << "[7] PASS\n";
        std::cerr << "[8] capture_survives_pointer_leaving_target\n";
        test_capture_survives_pointer_leaving_target();
        std::cerr << "[8] PASS\n";
        std::cerr << "[9] removed_captured_node_is_reconciled\n";
        test_removed_captured_node_is_reconciled();
        std::cerr << "[9] PASS\n";
        std::cerr << "[10] focus_callback_can_request_another_focus\n";
        test_focus_callback_can_request_another_focus();
        std::cerr << "[10] PASS\n";
    }
    catch (const TestFailure &failure)
    {
        std::cerr << "InputSystem test failed: " << failure.message << '\n';
        return EXIT_FAILURE;
    }

    std::cout << "InputSystem tests passed\n";
    return EXIT_SUCCESS;
}
