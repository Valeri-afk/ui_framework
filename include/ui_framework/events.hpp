#pragma once

#include <string>

#include "ui_framework/types.hpp"

namespace ui
{
    class Node;

    struct MousePosition
    {
        float x = 0.0f;
        float y = 0.0f;
    };

    enum class MouseButton : int
    {
        Unknown = 0,
        Left = 1,
        Middle = 2,
        Right = 3
    };

    struct KeyModifiers
    {
        bool shift = false;
        bool ctrl = false;
        bool alt = false;
        bool gui = false;
    };

    enum class KeyCode : int
    {
        UNKNOWN = 0,
        A, B, C, D, E, F, G, H, I, J, K, L, M,
        N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
        NUM_0, NUM_1, NUM_2, NUM_3, NUM_4, NUM_5, NUM_6, NUM_7, NUM_8, NUM_9,
        F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
        F13, F14, F15, F16, F17, F18, F19, F20, F21, F22, F23, F24,
        SPACE, ENTER, ESCAPE, TAB, BACKSPACE, DELETE, INSERT, HOME, END,
        PAGE_UP, PAGE_DOWN, UP, DOWN, LEFT, RIGHT,
        LSHIFT, RSHIFT, LCTRL, RCTRL, LALT, RALT, LGUI, RGUI,
        CAPS_LOCK, NUM_LOCK, SCROLL_LOCK, PAUSE, PRINT_SCREEN,
        COMMA, PERIOD, SLASH, SEMICOLON, QUOTE, LBRACKET, RBRACKET,
        BACKSLASH, GRAVE, MINUS, EQUALS
    };

    struct UIEvent
    {
        enum class Phase
        {
            TUNNELING,
            BUBBLING,
            TARGET
        };

        Node *target = nullptr;
        Node *currentTarget = nullptr;
        Phase phase = Phase::TUNNELING;
        bool propagationStopped = false;

        void stopPropagation() noexcept
        {
            propagationStopped = true;
        }
    };

    struct MouseMoveEvent : UIEvent { MousePosition position{}; };
    struct MouseDownEvent : UIEvent { MousePosition position{}; MouseButton button = MouseButton::Unknown; };
    struct MouseUpEvent : UIEvent { MousePosition position{}; MouseButton button = MouseButton::Unknown; };
    struct MouseClickEvent : UIEvent { MousePosition position{}; MouseButton button = MouseButton::Unknown; };
    struct MouseWheelEvent : UIEvent { MousePosition position{}; float scrolledX = 0.0f; float scrolledY = 0.0f; };
    struct MouseEnterEvent : UIEvent { MousePosition position{}; };
    struct MouseLeaveEvent : UIEvent { MousePosition position{}; };
    struct MouseDragBeginEvent : UIEvent { bool dragging = false; MousePosition position{}; LayoutSize delta{}; };
    struct MouseDragEvent : UIEvent { bool dragging = false; MousePosition position{}; LayoutSize delta{}; };
    struct MouseDragEndEvent : UIEvent { MousePosition position{}; LayoutSize delta{}; };
    struct KeyDownEvent : UIEvent { bool is_repeat = false; KeyCode key = KeyCode::UNKNOWN; KeyModifiers modifiers{}; };
    struct KeyUpEvent : UIEvent { bool is_repeat = false; KeyCode key = KeyCode::UNKNOWN; KeyModifiers modifiers{}; };
    struct FocusGainedEvent : UIEvent {};
    struct FocusLostEvent : UIEvent {};

    struct TextInputEvent : UIEvent
    {
        std::string text;
    };

    struct TextEditingEvent : UIEvent
    {
        std::string composition;
        int cursor = 0;
        int selectionLength = 0;
    };
}
