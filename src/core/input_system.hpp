#pragma once

#include <optional>

#include <SDL3/SDL.h>

#include "ui_framework/events.hpp"
#include "ui_framework/node.hpp"
#include "ui_framework/types.hpp"

namespace ui
{
    class NodeTree;

    class InputSystem
    {
    public:
        InputSystem();

        InputSystem(const InputSystem &) = delete;
        InputSystem &operator=(const InputSystem &) = delete;

        void processEvent(const SDL_Event &sdlEvent, NodeTree &nodeTree, const Node *modalRoot);
        void syncState(NodeTree &nodeTree);
        void resetState();
        void refreshHover(NodeTree &nodeTree, float x, float y, const Node *modalRoot);
        bool focus(NodeTree &nodeTree, Node &node);
        void clearFocus(NodeTree &nodeTree);
        bool capture(NodeTree &nodeTree, Node &node, std::optional<MousePosition> pressPosition = std::nullopt);
        void releaseCapture(NodeTree &nodeTree, std::optional<MousePosition> position = std::nullopt);
        void cancelPointerInteraction(NodeTree &nodeTree, std::optional<MousePosition> position = std::nullopt);
        void setModalRoot(const Node *modalRoot) noexcept;
        Node *focusedNode() const noexcept;
        std::optional<Node::Id> focusedNodeId() const noexcept;
        Node *capturedNode() const noexcept;
        Node *pressedNode() const noexcept;
        bool isDragging() const noexcept;

        // методы для тестов
        Node *hoveredNode() const noexcept;
        float getDragThreshhold() const noexcept { return input_.dragThreshold; };

    private:
        struct InputState
        {
            Node *hoveredNode = nullptr;
            Node *focusedNode = nullptr;
            Node *capturedNode = nullptr;
            Node *pressedNode = nullptr;
            std::optional<Node::Id> hoveredNodeId;
            std::optional<Node::Id> focusedNodeId;
            std::optional<Node::Id> capturedNodeId;
            std::optional<Node::Id> pressedNodeId;
            std::optional<MousePosition> pointerPosition_;
            bool isDragging = false;
            std::optional<MousePosition> pressPosition_;
            float dragThreshold = 5.0f;
        };

        InputState input_;
        std::optional<Node::Id> modalRootId_;
        std::optional<Node::Id> pendingFocusNodeId_;
        bool pendingClearFocus_ = false;
        bool focusTransitionInProgress_ = false;
        bool dragEndDispatchInProgress_ = false;

        static void rememberNode(Node *&node, std::optional<Node::Id> &id) noexcept;
        static void clearTrackedNode(Node *&node, std::optional<Node::Id> &id) noexcept;
        static void setTrackedNode(Node *&node, std::optional<Node::Id> &id, Node *newNode) noexcept;
        static void syncTrackedNode(Node *&node, std::optional<Node::Id> &id, NodeTree &nodeTree, bool requireEnabled, bool requireFocusable = false, bool requireCapturable = false);
        void validateInputState(NodeTree &nodeTree);
        void setModalRootId(const Node *modalRoot) noexcept;
        bool dispatchDragEndIfNeeded(NodeTree &nodeTree, Node *node, std::optional<MousePosition> position);
        bool dispatchMouseLeaveIfNeeded(NodeTree &nodeTree, Node *node, std::optional<MousePosition> position);
        void clearDragState() noexcept;
        void clearPointerTracking() noexcept;
        void handleMouseMoveEvent(Node *node, NodeTree &nodeTree, MouseMoveEvent &event);
        void handleMouseDownEvent(Node *node, NodeTree &nodeTree, MouseDownEvent &event, bool modalIsActive);
        void handleMouseUpEvent(Node *node, NodeTree &nodeTree, MouseUpEvent &event, bool modalIsActive);
        void handleMouseWheelEvent(Node *node, NodeTree &nodeTree, MouseWheelEvent &event);
        void handleKeyDownEvent(NodeTree &nodeTree, KeyDownEvent &event);
        void handleKeyUpEvent(NodeTree &nodeTree, KeyUpEvent &event);
        void handleTextInputEvent(NodeTree &nodeTree, TextInputEvent &event);
        void handleTextEditingEvent(NodeTree &nodeTree, TextEditingEvent &event);
        KeyCode convertSDLKeyCodeToKeyCode(SDL_Keycode sdlKey) const;
        Node *resolveModalRoot(NodeTree &nodeTree) const noexcept;
        bool isNodeAllowedByModal(NodeTree &nodeTree, Node *node) const noexcept;
    };
}
