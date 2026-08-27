#include "input_system.hpp"
#include "layout_system.hpp"
#include "node_tree.hpp"

#include <SDL3/SDL.h>

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

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

    SDL_Event mouseDown(float x, float y)
    {
        SDL_Event event{};
        event.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
        event.button.x = x;
        event.button.y = y;
        event.button.button = SDL_BUTTON_LEFT;
        return event;
    }

    SDL_Event mouseUp(float x, float y)
    {
        SDL_Event event{};
        event.type = SDL_EVENT_MOUSE_BUTTON_UP;
        event.button.x = x;
        event.button.y = y;
        event.button.button = SDL_BUTTON_LEFT;
        return event;
    }

    SDL_Event keyDown()
    {
        SDL_Event event{};
        event.type = SDL_EVENT_KEY_DOWN;
        event.key.key = SDLK_RETURN;
        return event;
    }

    SDL_Event mouseMotion(float x, float y)
    {
        SDL_Event event{};
        event.type = SDL_EVENT_MOUSE_MOTION;
        event.motion.x = x;
        event.motion.y = y;
        return event;
    }

    void test_mouse_down_removes_target()
    {
        Fixture f;
        f.root->setFocusable(true);
        f.root->setCapturable(true);
        bool callbackCalled = false;

        f.root->on<ui::MouseDownEvent>([&](ui::MouseDownEvent &, ui::Node &node)
                                       {
            callbackCalled = true;
            f.tree.removeRoot(&node); });

        f.input.processEvent(mouseDown(10.0f, 10.0f), f.tree, nullptr);

        expect(callbackCalled, "MouseDown callback must run before queued removal");
        expect(f.tree.rootsCount() == 0, "queued target removal must be flushed after dispatch");
        expect(f.input.pressedNode() == nullptr, "removed MouseDown target must clear pressed state");
        expect(f.input.capturedNode() == nullptr, "removed MouseDown target must not remain captured");
        expect(f.input.focusedNode() == nullptr, "removed MouseDown target must not remain focused");
        expect(!f.input.isDragging(), "removed MouseDown target must clear drag state");
    }

    void test_mouse_up_removes_captured_target()
    {
        Fixture f;
        f.root->setCapturable(true);
        expect(f.input.capture(f.tree, *f.root, ui::MousePosition{10.0f, 10.0f}),
               "capture setup must succeed");

        bool callbackCalled = false;
        f.root->on<ui::MouseUpEvent>([&](ui::MouseUpEvent &, ui::Node &node)
                                     {
            callbackCalled = true;
            f.tree.removeRoot(&node); });

        f.input.processEvent(mouseUp(10.0f, 10.0f), f.tree, nullptr);

        expect(callbackCalled, "MouseUp callback must run before queued removal");
        expect(f.tree.rootsCount() == 0, "captured target removal must be flushed");
        expect(f.input.capturedNode() == nullptr, "removed captured node must clear capture");
        expect(f.input.pressedNode() == nullptr, "removed captured node must clear pressed state");
        expect(!f.input.isDragging(), "removed captured node must clear drag state");
    }

    void test_key_down_removes_focused_target()
    {
        Fixture f;
        f.root->setFocusable(true);
        expect(f.input.focus(f.tree, *f.root), "focus setup must succeed");

        bool callbackCalled = false;
        f.root->on<ui::KeyDownEvent>([&](ui::KeyDownEvent &, ui::Node &node)
                                     {
            callbackCalled = true;
            f.tree.removeRoot(&node); });

        f.input.processEvent(keyDown(), f.tree, nullptr);

        expect(callbackCalled, "KeyDown callback must run before queued removal");
        expect(f.tree.rootsCount() == 0, "focused target removal must be flushed");
        expect(f.input.focusedNode() == nullptr, "removed focused node must clear focus");
    }

    void test_focus_lost_callback_can_request_another_focus()
    {
        Fixture f;

        f.root->setFocusable(true);

        auto second = std::make_unique<ui::Node>();
        second->setPosition({0.0f, 0.0f});
        second->setSize(ui::LayoutSizeValue::fixed(100.0f, 100.0f));
        second->setFocusable(true);

        ui::Node *secondPtr =
            f.tree.attachRoot(1, std::move(second));

        expect(f.input.focus(f.tree, *f.root),
               "initial focus setup must succeed");

        bool callbackCalled = false;
        bool nestedFocusReturned = false;

        f.root->on<ui::FocusLostEvent>(
            [&](ui::FocusLostEvent &, ui::Node &)
            {
                callbackCalled = true;
                nestedFocusReturned =
                    f.input.focus(f.tree, *secondPtr);
            });

        expect(f.input.focus(f.tree, *secondPtr),
               "focus transition must succeed");

        expect(callbackCalled,
               "FocusLost callback must run");

        expect(nestedFocusReturned,
               "nested focus request must be accepted");

        expect(f.input.focusedNode() == secondPtr,
               "second node must remain focused after reentrant focus");
    }

    void test_focus_lost_removes_old_target()
    {
        Fixture f;

        f.root->setFocusable(true);

        auto second = std::make_unique<ui::Node>();
        second->setPosition({0.0f, 0.0f});
        second->setSize(ui::LayoutSizeValue::fixed(100.0f, 100.0f));
        second->setFocusable(true);

        ui::Node *secondPtr =
            f.tree.attachRoot(1, std::move(second));

        const ui::Node::Id firstId = f.root->getId();

        expect(f.input.focus(f.tree, *f.root),
               "initial focus setup must succeed");

        bool callbackCalled = false;

        f.root->on<ui::FocusLostEvent>(
            [&](ui::FocusLostEvent &, ui::Node &node)
            {
                callbackCalled = true;
                f.tree.removeRoot(&node);
            });

        expect(f.input.focus(f.tree, *secondPtr),
               "focus must move to second node");

        expect(callbackCalled,
               "FocusLost callback must run before queued removal");
        expect(f.tree.findNode(firstId) == nullptr,
               "old focused node must be removed");
        expect(f.input.focusedNode() == secondPtr,
               "second node must remain focused");
    }

    void test_focus_gained_removes_focused_target()
    {
        Fixture f;
        f.root->setFocusable(true);

        bool callbackCalled = false;

        f.root->on<ui::FocusGainedEvent>(
            [&](ui::FocusGainedEvent &, ui::Node &node)
            {
                callbackCalled = true;
                f.tree.removeRoot(&node);
            });

        const bool result = f.input.focus(f.tree, *f.root);

        expect(callbackCalled,
               "FocusGained callback must run before queued removal");
        expect(f.tree.rootsCount() == 0,
               "FocusGained target removal must be flushed");
        expect(f.input.focusedNode() == nullptr,
               "removed FocusGained target must clear focus");
        expect(!result,
               "focus must fail when the requested node is removed during FocusGained");
    }

    void test_drag_end_callback_can_replace_capture()
    {
        Fixture f;
        f.root->setCapturable(true);

        auto replacement = std::make_unique<ui::Node>();
        replacement->setPosition({0.0f, 0.0f});
        replacement->setSize(ui::LayoutSizeValue::fixed(100.0f, 100.0f));
        replacement->setCapturable(true);
        ui::Node *replacementPtr = f.tree.attachRoot(1, std::move(replacement));

        bool dragEndCalled = false;
        f.root->on<ui::MouseDragEndEvent>([&](ui::MouseDragEndEvent &, ui::Node &)
                                          {
            dragEndCalled = true;
            f.input.capture(f.tree, *replacementPtr, ui::MousePosition{20.0f, 20.0f}); });

        f.input.processEvent(mouseDown(10.0f, 10.0f), f.tree, nullptr);
        f.input.processEvent(mouseMotion(30.0f, 10.0f), f.tree, nullptr);
        expect(f.input.isDragging(), "setup must enter drag state");

        f.input.processEvent(mouseUp(30.0f, 10.0f), f.tree, nullptr);

        expect(dragEndCalled, "DragEnd callback must run");
        expect(f.input.capturedNode() == replacementPtr,
               "capture created during DragEnd must not be overwritten by release cleanup");
    }
}

void test_focus_gained_callback_can_clear_focus()
{
    Fixture f;

    f.root->setFocusable(true);

    bool callbackCalled = false;

    f.root->on<ui::FocusGainedEvent>(
        [&](ui::FocusGainedEvent &, ui::Node &)
        {
            callbackCalled = true;
            f.input.clearFocus(f.tree);
        });

    const bool result = f.input.focus(f.tree, *f.root);

    expect(callbackCalled,
           "FocusGained callback must run");

    expect(f.input.focusedNode() == nullptr,
           "clearFocus requested from FocusGained must clear focus");

    expect(!result,
           "focus must report failure when FocusGained immediately clears focus");
}

void test_cancel_pointer_interaction_drag_end_removes_captured_target()
{
    Fixture f;

    f.root->setCapturable(true);

    expect(
        f.input.capture(
            f.tree,
            *f.root,
            ui::MousePosition{10.0f, 10.0f}),
        "capture setup must succeed");

    f.input.processEvent(
        mouseMotion(30.0f, 10.0f),
        f.tree,
        nullptr);

    expect(
        f.input.isDragging(),
        "setup must enter drag state");

    bool dragEndCalled = false;

    f.root->on<ui::MouseDragEndEvent>(
        [&](ui::MouseDragEndEvent &, ui::Node &node)
        {
            dragEndCalled = true;
            f.tree.removeRoot(&node);
        });

    f.input.cancelPointerInteraction(
        f.tree,
        ui::MousePosition{30.0f, 10.0f});

    expect(
        dragEndCalled,
        "DragEnd callback must run");

    expect(
        f.tree.rootsCount() == 0,
        "captured target removal must be flushed");

    expect(
        f.input.capturedNode() == nullptr,
        "removed captured node must clear capture");

    expect(
        f.input.pressedNode() == nullptr,
        "removed captured node must clear pressed state");

    expect(
        !f.input.isDragging(),
        "removed captured node must clear drag state");
}

void test_cancel_pointer_interaction_drag_end_can_replace_capture()
{
    Fixture f;

    f.root->setCapturable(true);

    auto replacement = std::make_unique<ui::Node>();
    replacement->setPosition({0.0f, 0.0f});
    replacement->setSize(
        ui::LayoutSizeValue::fixed(100.0f, 100.0f));
    replacement->setCapturable(true);

    ui::Node *replacementPtr =
        f.tree.attachRoot(1, std::move(replacement));

    expect(
        f.input.capture(
            f.tree,
            *f.root,
            ui::MousePosition{10.0f, 10.0f}),
        "capture setup must succeed");

    f.input.processEvent(
        mouseMotion(30.0f, 10.0f),
        f.tree,
        nullptr);

    expect(
        f.input.isDragging(),
        "setup must enter drag state");

    bool dragEndCalled = false;

    f.root->on<ui::MouseDragEndEvent>(
        [&](ui::MouseDragEndEvent &, ui::Node &)
        {
            dragEndCalled = true;

            expect(
                f.input.capture(
                    f.tree,
                    *replacementPtr,
                    ui::MousePosition{30.0f, 10.0f}),
                "replacement capture must succeed");
        });

    f.input.cancelPointerInteraction(
        f.tree,
        ui::MousePosition{30.0f, 10.0f});

    expect(
        dragEndCalled,
        "DragEnd callback must run");

    expect(
        f.input.capturedNode() == replacementPtr,
        "replacement capture must survive cancellation");

    expect(
        !f.input.isDragging(),
        "replacement capture must not inherit old drag state");
}

void test_cancel_pointer_interaction_mouse_leave_removes_hovered_target()
{
    Fixture f;

    bool leaveCalled = false;

    f.root->on<ui::MouseLeaveEvent>(
        [&](ui::MouseLeaveEvent &, ui::Node &node)
        {
            leaveCalled = true;
            f.tree.removeRoot(&node);
        });

    f.input.processEvent(
        mouseMotion(10.0f, 10.0f),
        f.tree,
        nullptr);

    expect(
        f.input.hoveredNode() == f.root,
        "setup must establish hovered node");

    f.input.cancelPointerInteraction(
        f.tree,
        ui::MousePosition{20.0f, 20.0f});

    expect(
        leaveCalled,
        "MouseLeave callback must run");

    expect(
        f.tree.rootsCount() == 0,
        "hovered target removal must be flushed");

    expect(
        f.input.hoveredNode() == nullptr,
        "removed hovered node must clear hover state");
}

void test_cancel_pointer_interaction_drag_end_reentrant_cancel()
{
    Fixture f;

    f.root->setCapturable(true);

    expect(
        f.input.capture(
            f.tree,
            *f.root,
            ui::MousePosition{10.0f, 10.0f}),
        "capture setup must succeed");

    f.input.processEvent(
        mouseMotion(30.0f, 10.0f),
        f.tree,
        nullptr);

    expect(
        f.input.isDragging(),
        "setup must enter drag state");

    int dragEndCalls = 0;

    f.root->on<ui::MouseDragEndEvent>(
        [&](ui::MouseDragEndEvent &, ui::Node &)
        {
            ++dragEndCalls;

            if (dragEndCalls == 1)
            {
                f.input.cancelPointerInteraction(
                    f.tree,
                    ui::MousePosition{30.0f, 10.0f});
            }
        });

    f.input.cancelPointerInteraction(
        f.tree,
        ui::MousePosition{30.0f, 10.0f});

    expect(
        dragEndCalls == 1,
        "reentrant cancellation must not dispatch DragEnd twice");

    expect(
        f.input.capturedNode() == nullptr,
        "capture must be cleared after reentrant cancellation");

    expect(
        f.input.pressedNode() == nullptr,
        "pressed state must be cleared after reentrant cancellation");

    expect(
        !f.input.isDragging(),
        "drag state must be cleared after reentrant cancellation");
}

void test_cancel_pointer_interaction_mouse_leave_can_capture()
{
    Fixture f;

    f.root->setCapturable(true);

    auto replacement = std::make_unique<ui::Node>();
    replacement->setPosition({0.0f, 0.0f});
    replacement->setSize(
        ui::LayoutSizeValue::fixed(100.0f, 100.0f));
    replacement->setCapturable(true);

    ui::Node *replacementPtr =
        f.tree.attachRoot(1, std::move(replacement));

    f.input.processEvent(
        mouseMotion(10.0f, 10.0f),
        f.tree,
        nullptr);

    expect(
        f.input.hoveredNode() == f.root,
        "setup must establish hover");

    bool leaveCalled = false;

    f.root->on<ui::MouseLeaveEvent>(
        [&](ui::MouseLeaveEvent &, ui::Node &)
        {
            leaveCalled = true;

            expect(
                f.input.capture(
                    f.tree,
                    *replacementPtr,
                    ui::MousePosition{10.0f, 10.0f}),
                "capture from MouseLeave must succeed");
        });

    f.input.cancelPointerInteraction(
        f.tree,
        ui::MousePosition{20.0f, 20.0f});

    expect(
        leaveCalled,
        "MouseLeave callback must run");

    expect(
        f.input.capturedNode() == replacementPtr,
        "capture created by MouseLeave must survive cancellation");

    expect(
        f.input.pressedNode() == replacementPtr,
        "replacement capture must establish pressed state");
}

void test_cancel_pointer_interaction_mouse_leave_removes_and_replaces_hover()
{
    Fixture f;

    auto replacement = std::make_unique<ui::Node>();
    replacement->setPosition({0.0f, 0.0f});
    replacement->setSize(
        ui::LayoutSizeValue::fixed(100.0f, 100.0f));

    ui::Node *replacementPtr =
        f.tree.attachRoot(1, std::move(replacement));

    f.input.processEvent(
        mouseMotion(10.0f, 10.0f),
        f.tree,
        nullptr);

    expect(
        f.input.hoveredNode() == f.root,
        "setup must establish initial hover");

    bool leaveCalled = false;

    f.root->on<ui::MouseLeaveEvent>(
        [&](ui::MouseLeaveEvent &, ui::Node &node)
        {
            leaveCalled = true;

            f.tree.removeRoot(&node);
        });

    f.input.cancelPointerInteraction(
        f.tree,
        ui::MousePosition{20.0f, 20.0f});

    expect(
        leaveCalled,
        "MouseLeave callback must run");

    expect(
        f.tree.findNode(f.root->getId()) == nullptr,
        "old hovered node must be removed");

    expect(
        f.input.hoveredNode() == nullptr,
        "stale hovered node must not survive callback mutation");

    (void)replacementPtr;
}

void test_cancel_pointer_interaction_drag_end_removes_and_replaces_capture()
{
    Fixture f;

    f.root->setCapturable(true);

    auto replacement = std::make_unique<ui::Node>();
    replacement->setPosition({0.0f, 0.0f});
    replacement->setSize(
        ui::LayoutSizeValue::fixed(100.0f, 100.0f));
    replacement->setCapturable(true);

    ui::Node *replacementPtr =
        f.tree.attachRoot(1, std::move(replacement));

    expect(
        f.input.capture(
            f.tree,
            *f.root,
            ui::MousePosition{10.0f, 10.0f}),
        "capture setup must succeed");

    f.input.processEvent(
        mouseMotion(30.0f, 10.0f),
        f.tree,
        nullptr);

    expect(
        f.input.isDragging(),
        "setup must enter drag state");

    bool dragEndCalled = false;

    f.root->on<ui::MouseDragEndEvent>(
        [&](ui::MouseDragEndEvent &, ui::Node &node)
        {
            dragEndCalled = true;

            f.tree.removeRoot(&node);

            expect(
                f.input.capture(
                    f.tree,
                    *replacementPtr,
                    ui::MousePosition{30.0f, 10.0f}),
                "replacement capture must succeed");
        });

    f.input.cancelPointerInteraction(
        f.tree,
        ui::MousePosition{30.0f, 10.0f});

    expect(
        dragEndCalled,
        "DragEnd callback must run");

    expect(
        f.tree.findNode(replacementPtr->getId()) == replacementPtr,
        "replacement node must remain alive");

    expect(
        f.input.capturedNode() == replacementPtr,
        "replacement capture must survive old target removal");

    expect(
        f.input.pressedNode() == replacementPtr,
        "replacement pressed state must survive old target removal");

    expect(
        !f.input.isDragging(),
        "replacement capture must not inherit old drag state");
}

void test_cancel_pointer_interaction_mouse_leave_can_replace_pressed_state()
{
    Fixture f;

    f.root->setCapturable(true);

    auto replacement = std::make_unique<ui::Node>();
    replacement->setPosition({0.0f, 0.0f});
    replacement->setSize(
        ui::LayoutSizeValue::fixed(100.0f, 100.0f));
    replacement->setCapturable(true);

    ui::Node *replacementPtr =
        f.tree.attachRoot(1, std::move(replacement));

    f.input.processEvent(
        mouseMotion(10.0f, 10.0f),
        f.tree,
        nullptr);

    expect(
        f.input.hoveredNode() == f.root,
        "setup must establish hover");

    bool leaveCalled = false;

    f.root->on<ui::MouseLeaveEvent>(
        [&](ui::MouseLeaveEvent &, ui::Node &)
        {
            leaveCalled = true;

            expect(
                f.input.capture(
                    f.tree,
                    *replacementPtr,
                    ui::MousePosition{20.0f, 20.0f}),
                "replacement capture must succeed");
        });

    f.input.cancelPointerInteraction(
        f.tree,
        ui::MousePosition{20.0f, 20.0f});

    expect(
        leaveCalled,
        "MouseLeave callback must run");

    expect(
        f.input.capturedNode() == replacementPtr,
        "capture created by MouseLeave must survive cancellation");

    expect(
        f.input.pressedNode() == replacementPtr,
        "pressed state created by MouseLeave must survive cancellation");
}

void test_capture_rejects_invalid_target()
{
    Fixture f;

    expect(
        !f.input.capture(
            f.tree,
            *f.root,
            ui::MousePosition{10.0f, 10.0f}),
        "capture must reject non-capturable node");

    expect(
        f.input.capturedNode() == nullptr,
        "failed capture must not create captured state");

    expect(
        f.input.pressedNode() == nullptr,
        "failed capture must not create pressed state");

    expect(
        !f.input.isDragging(),
        "failed capture must not create drag state");
}

void test_capture_rejects_node_outside_modal_root()
{
    Fixture f;

    f.root->setCapturable(true);

    auto modal = std::make_unique<ui::Node>();
    modal->setPosition({0.0f, 0.0f});
    modal->setSize(
        ui::LayoutSizeValue::fixed(50.0f, 50.0f));

    ui::Node *modalPtr =
        f.tree.attachRoot(1, std::move(modal));

    f.input.setModalRoot(modalPtr);

    expect(
        !f.input.capture(
            f.tree,
            *f.root,
            ui::MousePosition{10.0f, 10.0f}),
        "capture outside modal root must fail");

    expect(
        f.input.capturedNode() == nullptr,
        "failed modal capture must not create capture");

    expect(
        f.input.pressedNode() == nullptr,
        "failed modal capture must not create pressed state");
}

void test_capture_allows_node_inside_modal_root()
{
    Fixture f;

    auto modal = std::make_unique<ui::Node>();
    modal->setPosition({0.0f, 0.0f});
    modal->setSize(
        ui::LayoutSizeValue::fixed(100.0f, 100.0f));

    modal->setCapturable(true);

    ui::Node *modalPtr =
        f.tree.attachRoot(1, std::move(modal));

    f.input.setModalRoot(modalPtr);

    expect(
        f.input.capture(
            f.tree,
            *modalPtr,
            ui::MousePosition{10.0f, 10.0f}),
        "capture inside modal root must succeed");

    expect(
        f.input.capturedNode() == modalPtr,
        "modal node must become captured");
}

void test_capture_inside_modal_root_succeeds()
{
    Fixture f;

    auto modal = std::make_unique<ui::Node>();
    modal->setPosition({0.0f, 0.0f});
    modal->setSize(
        ui::LayoutSizeValue::fixed(100.0f, 100.0f));
    modal->setCapturable(true);

    ui::Node *modalPtr =
        f.tree.attachRoot(1, std::move(modal));

    f.input.setModalRoot(modalPtr);

    expect(
        f.input.capture(
            f.tree,
            *modalPtr,
            ui::MousePosition{10.0f, 10.0f}),
        "capture inside modal root must succeed");

    expect(
        f.input.capturedNode() == modalPtr,
        "modal root must become captured");

    expect(
        f.input.pressedNode() == modalPtr,
        "modal root must become pressed");
}

void test_captured_node_becoming_disabled_is_reconciled()
{
    Fixture f;

    f.root->setCapturable(true);

    expect(
        f.input.capture(
            f.tree,
            *f.root,
            ui::MousePosition{10.0f, 10.0f}),
        "capture setup must succeed");

    expect(
        f.input.capturedNode() == f.root,
        "root must initially be captured");

    f.root->setEnabled(false);

    f.input.processEvent(
        mouseMotion(20.0f, 20.0f),
        f.tree,
        nullptr);

    expect(
        f.input.capturedNode() == nullptr,
        "disabled captured node must lose capture");

    expect(
        f.input.pressedNode() == nullptr,
        "disabled captured node must lose pressed state");

    expect(
        !f.input.isDragging(),
        "disabled captured node must lose drag state");
}

void test_captured_node_becoming_invisible_is_reconciled()
{
    Fixture f;

    f.root->setCapturable(true);

    expect(
        f.input.capture(
            f.tree,
            *f.root,
            ui::MousePosition{10.0f, 10.0f}),
        "capture setup must succeed");

    f.root->setVisible(false);

    f.input.processEvent(
        mouseMotion(20.0f, 20.0f),
        f.tree,
        nullptr);

    expect(
        f.input.capturedNode() == nullptr,
        "invisible captured node must lose capture");

    expect(
        f.input.pressedNode() == nullptr,
        "invisible captured node must lose pressed state");

    expect(
        !f.input.isDragging(),
        "invisible captured node must lose drag state");
}

void test_captured_node_becoming_non_capturable_is_reconciled()
{
    Fixture f;

    f.root->setCapturable(true);

    expect(
        f.input.capture(
            f.tree,
            *f.root,
            ui::MousePosition{10.0f, 10.0f}),
        "capture setup must succeed");

    f.root->setCapturable(false);

    f.input.processEvent(
        mouseMotion(20.0f, 20.0f),
        f.tree,
        nullptr);

    expect(
        f.input.capturedNode() == nullptr,
        "non-capturable captured node must lose capture");

    expect(
        f.input.pressedNode() == nullptr,
        "non-capturable captured node must lose pressed state");

    expect(
        !f.input.isDragging(),
        "non-capturable captured node must lose drag state");
}

void test_captured_node_outside_new_modal_root_is_reconciled()
{
    Fixture f;

    f.root->setCapturable(true);

    auto modal = std::make_unique<ui::Node>();
    modal->setPosition({0.0f, 0.0f});
    modal->setSize(
        ui::LayoutSizeValue::fixed(100.0f, 100.0f));
    modal->setCapturable(true);

    ui::Node *modalPtr =
        f.tree.attachRoot(1, std::move(modal));

    expect(
        f.input.capture(
            f.tree,
            *f.root,
            ui::MousePosition{10.0f, 10.0f}),
        "initial capture must succeed");

    expect(
        f.input.capturedNode() == f.root,
        "root must initially be captured");

    f.input.setModalRoot(modalPtr);

    f.input.processEvent(
        mouseMotion(20.0f, 20.0f),
        f.tree,
        modalPtr);

    expect(
        f.input.capturedNode() == nullptr,
        "capture outside new modal root must be cleared");

    expect(
        f.input.pressedNode() == nullptr,
        "pressed state outside new modal root must be cleared");

    expect(
        !f.input.isDragging(),
        "drag state outside new modal root must be cleared");
}

void test_removed_modal_root_reconciles_capture()
{
    Fixture f;

    auto modal = std::make_unique<ui::Node>();
    modal->setPosition({0.0f, 0.0f});
    modal->setSize(
        ui::LayoutSizeValue::fixed(100.0f, 100.0f));
    modal->setCapturable(true);

    ui::Node *modalPtr =
        f.tree.attachRoot(1, std::move(modal));

    f.input.setModalRoot(modalPtr);

    expect(
        f.input.capture(
            f.tree,
            *modalPtr,
            ui::MousePosition{10.0f, 10.0f}),
        "modal capture setup must succeed");

    const ui::Node::Id modalId = modalPtr->getId();

    f.tree.removeRoot(modalPtr);
    f.tree.flushMutationQueue();

    expect(
        f.tree.findNode(modalId) == nullptr,
        "modal root must be removed");

    f.input.processEvent(
        mouseMotion(20.0f, 20.0f),
        f.tree,
        nullptr);

    expect(
        f.input.capturedNode() == nullptr,
        "removed modal target must lose capture");

    expect(
        f.input.pressedNode() == nullptr,
        "removed modal target must lose pressed state");

    expect(
        !f.input.isDragging(),
        "removed modal target must lose drag state");
}

void test_capture_requires_visible_enabled_target()
{
    Fixture f;

    f.root->setCapturable(true);

    f.root->setVisible(false);

    expect(
        !f.input.capture(
            f.tree,
            *f.root,
            ui::MousePosition{10.0f, 10.0f}),
        "capture must reject invisible node");

    f.root->setVisible(true);
    f.root->setEnabled(false);

    expect(
        !f.input.capture(
            f.tree,
            *f.root,
            ui::MousePosition{10.0f, 10.0f}),
        "capture must reject disabled node");

    expect(
        f.input.capturedNode() == nullptr,
        "invalid capture attempts must leave capture empty");
}

void test_capture_sets_pressed_and_capture_state()
{
    Fixture f;

    f.root->setCapturable(true);

    const ui::MousePosition position{15.0f, 25.0f};

    expect(
        f.input.capture(f.tree, *f.root, position),
        "capture must succeed");

    expect(
        f.input.capturedNode() == f.root,
        "capture must set captured node");

    expect(
        f.input.pressedNode() == f.root,
        "capture must set pressed node");

    expect(
        !f.input.isDragging(),
        "capture must not immediately enter drag state");
}

void test_capture_replaces_existing_capture()
{
    Fixture f;

    f.root->setCapturable(true);

    auto second = std::make_unique<ui::Node>();
    second->setPosition({0.0f, 0.0f});
    second->setSize(
        ui::LayoutSizeValue::fixed(100.0f, 100.0f));
    second->setCapturable(true);

    ui::Node *secondPtr =
        f.tree.attachRoot(1, std::move(second));

    expect(
        f.input.capture(
            f.tree,
            *f.root,
            ui::MousePosition{10.0f, 10.0f}),
        "initial capture must succeed");

    expect(
        f.input.capturedNode() == f.root,
        "initial target must be captured");

    expect(
        f.input.capture(
            f.tree,
            *secondPtr,
            ui::MousePosition{20.0f, 20.0f}),
        "replacement capture must succeed");

    expect(
        f.input.capturedNode() == secondPtr,
        "new target must replace old capture");

    expect(
        f.input.pressedNode() == secondPtr,
        "new target must replace pressed state");

    expect(
        !f.input.isDragging(),
        "replacement capture must not inherit drag state");
}

void test_capture_rejects_removed_target()
{
    Fixture f;

    f.root->setCapturable(true);

    ui::Node *target = f.root;

    f.tree.removeRoot(target);
    f.tree.flushMutationQueue();

    expect(
        f.tree.findNode(target->getId()) == nullptr,
        "target must be removed before capture");

    // Do not call capture() with the dangling target pointer.
    // The API must be tested through a live node only.
    expect(
        f.input.capturedNode() == nullptr,
        "removed target must not be captured");
}

void test_capture_from_existing_capture_after_old_target_removal()
{
    Fixture f;

    f.root->setCapturable(true);

    auto second = std::make_unique<ui::Node>();
    second->setPosition({0.0f, 0.0f});
    second->setSize(
        ui::LayoutSizeValue::fixed(100.0f, 100.0f));
    second->setCapturable(true);

    ui::Node *secondPtr =
        f.tree.attachRoot(1, std::move(second));

    expect(
        f.input.capture(
            f.tree,
            *f.root,
            ui::MousePosition{10.0f, 10.0f}),
        "initial capture must succeed");

    const ui::Node::Id firstId = f.root->getId();

    f.tree.removeRoot(f.root);
    f.tree.flushMutationQueue();

    expect(
        f.tree.findNode(firstId) == nullptr,
        "old captured node must be removed");

    f.input.syncState(f.tree);

    expect(
        f.input.capturedNode() == nullptr,
        "removed capture must be reconciled");

    expect(
        f.input.capture(
            f.tree,
            *secondPtr,
            ui::MousePosition{20.0f, 20.0f}),
        "capture of replacement node must succeed");

    expect(
        f.input.capturedNode() == secondPtr,
        "replacement node must become captured");
}

void test_capture_replacement_during_drag_end()
{
    Fixture f;

    f.root->setCapturable(true);

    auto second = std::make_unique<ui::Node>();
    second->setPosition({0.0f, 0.0f});
    second->setSize(
        ui::LayoutSizeValue::fixed(100.0f, 100.0f));
    second->setCapturable(true);

    ui::Node *secondPtr =
        f.tree.attachRoot(1, std::move(second));

    expect(
        f.input.capture(
            f.tree,
            *f.root,
            ui::MousePosition{10.0f, 10.0f}),
        "initial capture must succeed");

    f.input.processEvent(
        mouseMotion(30.0f, 10.0f),
        f.tree,
        nullptr);

    expect(
        f.input.isDragging(),
        "capture must enter drag state after threshold");

    bool callbackCalled = false;

    f.root->on<ui::MouseDragEndEvent>(
        [&](ui::MouseDragEndEvent &, ui::Node &)
        {
            callbackCalled = true;

            expect(
                f.input.capture(
                    f.tree,
                    *secondPtr,
                    ui::MousePosition{30.0f, 10.0f}),
                "replacement capture must succeed");
        });

    f.input.releaseCapture(
        f.tree,
        ui::MousePosition{30.0f, 10.0f});

    expect(
        callbackCalled,
        "DragEnd callback must run");

    expect(
        f.input.capturedNode() == secondPtr,
        "capture created during DragEnd must survive release");

    expect(
        f.input.pressedNode() == secondPtr,
        "replacement capture must establish pressed state");

    expect(
        !f.input.isDragging(),
        "replacement capture must not inherit previous drag state");
}

void test_drag_begin_removes_captured_target()
{
    Fixture f;

    f.root->setCapturable(true);

    expect(
        f.input.capture(
            f.tree,
            *f.root,
            ui::MousePosition{10.0f, 10.0f}),
        "capture setup must succeed");

    bool dragBeginCalled = false;

    f.root->on<ui::MouseDragBeginEvent>(
        [&](ui::MouseDragBeginEvent &, ui::Node &node)
        {
            dragBeginCalled = true;
            f.tree.removeRoot(&node);
        });

    f.input.processEvent(
        mouseMotion(30.0f, 10.0f),
        f.tree,
        nullptr);

    expect(
        dragBeginCalled,
        "DragBegin callback must run");

    expect(
        f.tree.rootsCount() == 0,
        "DragBegin target removal must be flushed");

    expect(
        f.input.capturedNode() == nullptr,
        "removed DragBegin target must clear capture");

    expect(
        f.input.pressedNode() == nullptr,
        "removed DragBegin target must clear pressed state");

    expect(
        !f.input.isDragging(),
        "removed DragBegin target must clear drag state");
}

void test_drag_begin_can_replace_capture()
{
    Fixture f;

    f.root->setCapturable(true);

    auto replacement = std::make_unique<ui::Node>();
    replacement->setPosition({0.0f, 0.0f});
    replacement->setSize(
        ui::LayoutSizeValue::fixed(100.0f, 100.0f));
    replacement->setCapturable(true);

    ui::Node *replacementPtr =
        f.tree.attachRoot(1, std::move(replacement));

    bool dragBeginCalled = false;

    f.root->on<ui::MouseDragBeginEvent>(
        [&](ui::MouseDragBeginEvent &, ui::Node &)
        {
            dragBeginCalled = true;

            expect(
                f.input.capture(
                    f.tree,
                    *replacementPtr,
                    ui::MousePosition{30.0f, 10.0f}),
                "replacement capture from DragBegin must succeed");
        });

    expect(
        f.input.capture(
            f.tree,
            *f.root,
            ui::MousePosition{10.0f, 10.0f}),
        "initial capture must succeed");

    f.input.processEvent(
        mouseMotion(30.0f, 10.0f),
        f.tree,
        nullptr);

    expect(
        dragBeginCalled,
        "DragBegin callback must run");

    expect(
        f.input.capturedNode() == replacementPtr,
        "replacement capture must survive DragBegin");

    expect(
        f.input.pressedNode() == replacementPtr,
        "replacement capture must replace pressed state");

    expect(
        !f.input.isDragging(),
        "replacement capture must not inherit drag state");
}

void test_drag_event_removes_captured_target()
{
    Fixture f;

    f.root->setCapturable(true);

    expect(
        f.input.capture(
            f.tree,
            *f.root,
            ui::MousePosition{10.0f, 10.0f}),
        "capture setup must succeed");

    bool dragCalled = false;

    f.root->on<ui::MouseDragEvent>(
        [&](ui::MouseDragEvent &, ui::Node &node)
        {
            dragCalled = true;
            f.tree.removeRoot(&node);
        });

    // First motion crosses threshold and dispatches DragBegin.
    // The Drag event is then dispatched during the same motion.
    f.input.processEvent(
        mouseMotion(30.0f, 10.0f),
        f.tree,
        nullptr);

    expect(
        dragCalled,
        "Drag callback must run");

    expect(
        f.tree.rootsCount() == 0,
        "Drag target removal must be flushed");

    expect(
        f.input.capturedNode() == nullptr,
        "removed Drag target must clear capture");

    expect(
        f.input.pressedNode() == nullptr,
        "removed Drag target must clear pressed state");

    expect(
        !f.input.isDragging(),
        "removed Drag target must clear drag state");
}

void test_drag_event_can_replace_capture()
{
    Fixture f;

    f.root->setCapturable(true);

    auto replacement = std::make_unique<ui::Node>();
    replacement->setPosition({0.0f, 0.0f});
    replacement->setSize(
        ui::LayoutSizeValue::fixed(100.0f, 100.0f));
    replacement->setCapturable(true);

    ui::Node *replacementPtr =
        f.tree.attachRoot(1, std::move(replacement));

    bool dragCalled = false;

    f.root->on<ui::MouseDragEvent>(
        [&](ui::MouseDragEvent &, ui::Node &)
        {
            dragCalled = true;

            expect(
                f.input.capture(
                    f.tree,
                    *replacementPtr,
                    ui::MousePosition{30.0f, 10.0f}),
                "replacement capture from Drag must succeed");
        });

    expect(
        f.input.capture(
            f.tree,
            *f.root,
            ui::MousePosition{10.0f, 10.0f}),
        "initial capture must succeed");

    f.input.processEvent(
        mouseMotion(30.0f, 10.0f),
        f.tree,
        nullptr);

    expect(
        dragCalled,
        "Drag callback must run");

    expect(
        f.input.capturedNode() == replacementPtr,
        "replacement capture must survive Drag");

    expect(
        f.input.pressedNode() == replacementPtr,
        "replacement capture must replace pressed state");

    expect(
        !f.input.isDragging(),
        "replacement capture must not inherit old drag state");
}

void test_drag_above_threshold_starts_drag()
{
    Fixture f;

    f.root->setCapturable(true);

    expect(
        f.input.capture(
            f.tree,
            *f.root,
            ui::MousePosition{10.0f, 10.0f}),
        "capture setup must succeed");

    bool dragBeginCalled = false;

    f.root->on<ui::MouseDragBeginEvent>(
        [&](ui::MouseDragBeginEvent &, ui::Node &)
        {
            dragBeginCalled = true;
        });

    const float threshold = f.input.getDragThreshhold();

    f.input.processEvent(
        mouseMotion(
            10.0f + threshold + 0.1f,
            10.0f),
        f.tree,
        nullptr);

    expect(
        dragBeginCalled,
        "DragBegin must run above threshold");

    expect(
        f.input.isDragging(),
        "movement above threshold must start drag");
}

void test_drag_threshold_boundary_does_not_start_drag()
{
    Fixture f;

    f.root->setCapturable(true);

    expect(
        f.input.capture(
            f.tree,
            *f.root,
            ui::MousePosition{10.0f, 10.0f}),
        "capture setup must succeed");

    bool dragBeginCalled = false;

    f.root->on<ui::MouseDragBeginEvent>(
        [&](ui::MouseDragBeginEvent &, ui::Node &)
        {
            dragBeginCalled = true;
        });

    const float threshold = f.input.getDragThreshhold();

    f.input.processEvent(
        mouseMotion(10.0f + threshold, 10.0f),
        f.tree,
        nullptr);

    expect(
        !f.input.isDragging(),
        "movement exactly at threshold must not start drag");

    expect(
        !dragBeginCalled,
        "DragBegin must not run exactly at threshold");
}

void test_drag_end_is_dispatched_once()
{
    Fixture f;

    f.root->setCapturable(true);

    expect(
        f.input.capture(
            f.tree,
            *f.root,
            ui::MousePosition{10.0f, 10.0f}),
        "capture setup must succeed");

    f.input.processEvent(
        mouseMotion(30.0f, 10.0f),
        f.tree,
        nullptr);

    expect(
        f.input.isDragging(),
        "setup must enter drag state");

    int dragEndCount = 0;

    f.root->on<ui::MouseDragEndEvent>(
        [&](ui::MouseDragEndEvent &, ui::Node &)
        {
            ++dragEndCount;
        });

    f.input.releaseCapture(
        f.tree,
        ui::MousePosition{30.0f, 10.0f});

    f.input.releaseCapture(
        f.tree,
        ui::MousePosition{30.0f, 10.0f});

    expect(
        dragEndCount == 1,
        "DragEnd must be dispatched exactly once");

    expect(
        f.input.capturedNode() == nullptr,
        "release must clear capture");

    expect(
        !f.input.isDragging(),
        "release must clear drag state");
}

void test_cancel_without_drag_does_not_dispatch_drag_end()
{
    Fixture f;

    f.root->setCapturable(true);

    expect(
        f.input.capture(
            f.tree,
            *f.root,
            ui::MousePosition{10.0f, 10.0f}),
        "capture setup must succeed");

    int dragEndCount = 0;

    f.root->on<ui::MouseDragEndEvent>(
        [&](ui::MouseDragEndEvent &, ui::Node &)
        {
            ++dragEndCount;
        });

    f.input.cancelPointerInteraction(
        f.tree,
        ui::MousePosition{20.0f, 20.0f});

    expect(
        dragEndCount == 0,
        "cancel without drag must not dispatch DragEnd");

    expect(
        f.input.capturedNode() == nullptr,
        "cancel must clear capture");

    expect(
        f.input.pressedNode() == nullptr,
        "cancel must clear pressed state");

    expect(
        !f.input.isDragging(),
        "cancel must clear drag state");
}

void test_release_without_drag_does_not_dispatch_drag_end()
{
    Fixture f;

    f.root->setCapturable(true);

    expect(
        f.input.capture(
            f.tree,
            *f.root,
            ui::MousePosition{10.0f, 10.0f}),
        "capture setup must succeed");

    int dragEndCount = 0;

    f.root->on<ui::MouseDragEndEvent>(
        [&](ui::MouseDragEndEvent &, ui::Node &)
        {
            ++dragEndCount;
        });

    f.input.releaseCapture(
        f.tree,
        ui::MousePosition{20.0f, 20.0f});

    expect(
        dragEndCount == 0,
        "release without drag must not dispatch DragEnd");

    expect(
        f.input.capturedNode() == nullptr,
        "release must clear capture");

    expect(
        f.input.pressedNode() == nullptr,
        "release must clear pressed state");

    expect(
        !f.input.isDragging(),
        "release must clear drag state");
}

void test_capture_rejects_invisible_node()
{
    Fixture f;

    f.root->setCapturable(true);
    f.root->setVisible(false);

    const bool result =
        f.input.capture(
            f.tree,
            *f.root,
            ui::MousePosition{10.0f, 10.0f});

    expect(
        !result,
        "capture must reject invisible node");

    expect(
        f.input.capturedNode() == nullptr,
        "invisible node must not become captured");

    expect(
        f.input.pressedNode() == nullptr,
        "invisible node must not become pressed");
}

void test_capture_rejects_disabled_node()
{
    Fixture f;

    f.root->setCapturable(true);
    f.root->setEnabled(false);

    const bool result =
        f.input.capture(
            f.tree,
            *f.root,
            ui::MousePosition{10.0f, 10.0f});

    expect(
        !result,
        "capture must reject disabled node");

    expect(
        f.input.capturedNode() == nullptr,
        "disabled node must not become captured");

    expect(
        f.input.pressedNode() == nullptr,
        "disabled node must not become pressed");
}

void test_capture_rejects_non_capturable_node()
{
    Fixture f;

    f.root->setCapturable(false);

    const bool result =
        f.input.capture(
            f.tree,
            *f.root,
            ui::MousePosition{10.0f, 10.0f});

    expect(
        !result,
        "capture must reject non-capturable node");

    expect(
        f.input.capturedNode() == nullptr,
        "non-capturable node must not become captured");

    expect(
        f.input.pressedNode() == nullptr,
        "non-capturable node must not become pressed");
}

void test_capture_rejects_detached_node()
{
    Fixture f;

    f.root->setCapturable(true);

    ui::Node *detached = f.root;

    f.tree.removeRoot(f.root);
    f.tree.flushMutationQueue();

    const bool result =
        f.input.capture(
            f.tree,
            *detached,
            ui::MousePosition{10.0f, 10.0f});

    expect(
        !result,
        "capture must reject node that is no longer in NodeTree");

    expect(
        f.input.capturedNode() == nullptr,
        "detached node must not become captured");

    expect(
        f.input.pressedNode() == nullptr,
        "detached node must not become pressed");
}

void test_capture_same_node_resets_press_state()
{
    Fixture f;

    f.root->setCapturable(true);

    expect(
        f.input.capture(
            f.tree,
            *f.root,
            ui::MousePosition{10.0f, 10.0f}),
        "initial capture must succeed");

    f.input.processEvent(
        mouseMotion(30.0f, 10.0f),
        f.tree,
        nullptr);

    expect(
        f.input.isDragging(),
        "setup must enter drag state");

    expect(
        f.input.capture(
            f.tree,
            *f.root,
            ui::MousePosition{50.0f, 50.0f}),
        "recapture of same node must succeed");

    expect(
        f.input.capturedNode() == f.root,
        "same node must remain captured");

    expect(
        f.input.pressedNode() == f.root,
        "same node must remain pressed");

    expect(
        !f.input.isDragging(),
        "recapture must reset drag state");
}

void test_capture_replacement_during_drag_end_survives()
{
    Fixture f;

    f.root->setCapturable(true);

    auto second = std::make_unique<ui::Node>();
    second->setPosition({0.0f, 0.0f});
    second->setSize(
        ui::LayoutSizeValue::fixed(100.0f, 100.0f));
    second->setCapturable(true);

    ui::Node *secondPtr =
        f.tree.attachRoot(1, std::move(second));

    expect(
        f.input.capture(
            f.tree,
            *f.root,
            ui::MousePosition{10.0f, 10.0f}),
        "initial capture must succeed");

    f.input.processEvent(
        mouseMotion(30.0f, 10.0f),
        f.tree,
        nullptr);

    expect(
        f.input.isDragging(),
        "setup must enter drag state");

    bool dragEndCalled = false;

    f.root->on<ui::MouseDragEndEvent>(
        [&](ui::MouseDragEndEvent &, ui::Node &)
        {
            dragEndCalled = true;

            expect(
                f.input.capture(
                    f.tree,
                    *secondPtr,
                    ui::MousePosition{40.0f, 40.0f}),
                "replacement capture from DragEnd must succeed");
        });

    f.input.processEvent(
        mouseUp(30.0f, 10.0f),
        f.tree,
        nullptr);

    expect(
        dragEndCalled,
        "DragEnd callback must run");

    expect(
        f.input.capturedNode() == secondPtr,
        "replacement capture must survive release");

    expect(
        f.input.pressedNode() == secondPtr,
        "replacement pressed node must survive release");

    expect(
        !f.input.isDragging(),
        "replacement capture must not inherit old drag state");
}

void test_capture_replacement_mouse_leave_can_replace_capture()
{
    Fixture f;

    f.root->setCapturable(true);

    auto second = std::make_unique<ui::Node>();
    second->setPosition({0.0f, 0.0f});
    second->setSize(
        ui::LayoutSizeValue::fixed(100.0f, 100.0f));
    second->setCapturable(true);

    ui::Node *secondPtr =
        f.tree.attachRoot(1, std::move(second));

    f.input.processEvent(
        mouseMotion(10.0f, 10.0f),
        f.tree,
        nullptr);

    expect(
        f.input.hoveredNode() == f.root,
        "setup must establish hover");

    bool leaveCalled = false;

    f.root->on<ui::MouseLeaveEvent>(
        [&](ui::MouseLeaveEvent &, ui::Node &)
        {
            leaveCalled = true;

            expect(
                f.input.capture(
                    f.tree,
                    *secondPtr,
                    ui::MousePosition{20.0f, 20.0f}),
                "capture from MouseLeave must succeed");
        });

    f.input.cancelPointerInteraction(
        f.tree,
        ui::MousePosition{20.0f, 20.0f});

    expect(
        leaveCalled,
        "MouseLeave callback must run");

    expect(
        f.input.capturedNode() == secondPtr,
        "capture created by MouseLeave must survive cancellation");

    expect(
        f.input.pressedNode() == secondPtr,
        "pressed node must follow MouseLeave capture");

    expect(
        !f.input.isDragging(),
        "MouseLeave capture must not start dragging");
}

void test_capture_target_removed_before_next_input()
{
    Fixture f;

    f.root->setCapturable(true);

    expect(
        f.input.capture(
            f.tree,
            *f.root,
            ui::MousePosition{10.0f, 10.0f}),
        "initial capture must succeed");

    const ui::Node::Id firstId = f.root->getId();

    f.tree.removeRoot(f.root);
    f.tree.flushMutationQueue();

    expect(
        f.tree.findNode(firstId) == nullptr,
        "captured node must be removed from tree");

    // Важно: между удалением Node и следующим обращением
    // к InputSystem ничего не вызываем.
    f.input.processEvent(
        mouseMotion(20.0f, 20.0f),
        f.tree,
        nullptr);

    expect(
        f.input.capturedNode() == nullptr,
        "removed capture must be reconciled on next input");

    expect(
        f.input.pressedNode() == nullptr,
        "removed pressed node must be reconciled on next input");

    expect(
        !f.input.isDragging(),
        "removed capture must clear drag state on next input");
}
void test_capture_sets_pressed_node_to_target()
{
    Fixture f;

    f.root->setCapturable(true);

    expect(
        f.input.capture(
            f.tree,
            *f.root,
            ui::MousePosition{15.0f, 25.0f}),
        "capture must succeed");

    expect(
        f.input.capturedNode() == f.root,
        "capture must set captured node");

    expect(
        f.input.pressedNode() == f.root,
        "capture must set pressed node");

    expect(
        !f.input.isDragging(),
        "capture must initially have no drag state");
}

void test_capture_without_press_position()
{
    Fixture f;

    f.root->setCapturable(true);

    expect(
        f.input.capture(
            f.tree,
            *f.root),
        "capture without press position must succeed");

    expect(
        f.input.capturedNode() == f.root,
        "node must become captured");

    expect(
        f.input.pressedNode() == f.root,
        "node must become pressed");

    expect(
        !f.input.isDragging(),
        "capture without press position must not start dragging");

    f.input.processEvent(
        mouseMotion(20.0f, 20.0f),
        f.tree,
        nullptr);

    expect(
        f.input.capturedNode() == f.root,
        "capture must survive motion without press position");

    expect(
        f.input.pressedNode() == f.root,
        "pressed node must survive motion without press position");
}

void test_capture_rejects_node_not_in_tree()
{
    Fixture f;

    auto detached = std::make_unique<ui::Node>();
    detached->setCapturable(true);

    ui::Node *detachedPtr = detached.get();

    const bool result =
        f.input.capture(
            f.tree,
            *detachedPtr,
            ui::MousePosition{10.0f, 10.0f});

    expect(
        !result,
        "capture must reject node that is not in NodeTree");

    expect(
        f.input.capturedNode() == nullptr,
        "detached node must not become captured");

    expect(
        f.input.pressedNode() == nullptr,
        "detached node must not become pressed");
}

void test_capture_target_removed_before_next_capture()
{
    Fixture f;

    f.root->setCapturable(true);

    expect(
        f.input.capture(
            f.tree,
            *f.root,
            ui::MousePosition{10.0f, 10.0f}),
        "initial capture must succeed");

    const ui::Node::Id capturedId = f.root->getId();

    f.tree.removeRoot(f.root);

    auto replacement = std::make_unique<ui::Node>();
    replacement->setCapturable(true);

    ui::Node *replacementPtr =
        f.tree.attachRoot(1, std::move(replacement));

    expect(
        f.input.capture(
            f.tree,
            *replacementPtr,
            ui::MousePosition{20.0f, 20.0f}),
        "replacement capture must succeed");

    expect(
        f.tree.findNode(capturedId) == nullptr,
        "old captured node must be removed");

    expect(
        f.input.capturedNode() == replacementPtr,
        "replacement node must become captured");
}

int main()
{
    try
    {
        test_mouse_down_removes_target();
        test_mouse_up_removes_captured_target();
        test_key_down_removes_focused_target();
        test_drag_end_callback_can_replace_capture();

        test_focus_gained_removes_focused_target();
        test_focus_lost_removes_old_target();
        test_focus_lost_callback_can_request_another_focus();
        test_focus_gained_callback_can_clear_focus();

        test_cancel_pointer_interaction_drag_end_removes_captured_target();
        test_cancel_pointer_interaction_drag_end_can_replace_capture();
        test_cancel_pointer_interaction_mouse_leave_removes_hovered_target();

        test_cancel_pointer_interaction_drag_end_reentrant_cancel();
        test_cancel_pointer_interaction_mouse_leave_can_capture();
        test_cancel_pointer_interaction_mouse_leave_removes_and_replaces_hover();
        test_cancel_pointer_interaction_drag_end_removes_and_replaces_capture();

        test_cancel_pointer_interaction_mouse_leave_can_replace_pressed_state();

        test_capture_rejects_invalid_target();
        test_capture_requires_visible_enabled_target();
        test_capture_sets_pressed_and_capture_state();
        test_capture_replaces_existing_capture();
        test_capture_replacement_during_drag_end();

        test_capture_rejects_node_outside_modal_root();
        test_capture_allows_node_inside_modal_root();
        test_capture_inside_modal_root_succeeds();

        test_captured_node_becoming_disabled_is_reconciled();
        test_captured_node_becoming_invisible_is_reconciled();
        test_captured_node_becoming_non_capturable_is_reconciled();

        test_captured_node_outside_new_modal_root_is_reconciled();
        test_removed_modal_root_reconciles_capture();

        test_drag_begin_removes_captured_target();
        test_drag_begin_can_replace_capture();

        test_drag_event_removes_captured_target();
        test_drag_event_can_replace_capture();

        test_drag_threshold_boundary_does_not_start_drag();
        test_drag_above_threshold_starts_drag();

        test_drag_end_is_dispatched_once();

        test_cancel_without_drag_does_not_dispatch_drag_end();
        test_release_without_drag_does_not_dispatch_drag_end();

        test_capture_rejects_invisible_node();
        test_capture_rejects_disabled_node();
        test_capture_rejects_non_capturable_node();
        test_capture_rejects_detached_node();

        test_capture_same_node_resets_press_state();
        test_capture_replacement_during_drag_end_survives();
        test_capture_replacement_mouse_leave_can_replace_capture();

        test_capture_target_removed_before_next_capture();
        test_capture_target_removed_before_next_input();

        test_capture_sets_pressed_node_to_target();
        test_capture_without_press_position();

        test_capture_rejects_node_not_in_tree();
    }
    catch (const TestFailure &failure)
    {
        std::cerr << "InputSystem mutation test failed: " << failure.message << '\n';
        return EXIT_FAILURE;
    }

    std::cout << "InputSystem mutation tests passed\n";
    return EXIT_SUCCESS;
}
