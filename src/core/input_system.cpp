#include "input_system.hpp"
#include "node_tree.hpp"
#include "event_dispatcher.hpp"

#include <algorithm>

namespace ui
{

    namespace
    {
        template <typename Event>
        bool dispatchEvent(
            NodeTree &nodeTree,
            Node *target,
            Event &event,
            bool tunneling,
            bool bubbling)
        {
            if (!target)
                return false;

            const Node::Id targetId = target->getId();

            if (nodeTree.findNode(targetId) != target)
                return false;

            {
                NodeTree::ScopedMutationGuard guard(nodeTree);

                EventDispatcher::dispatch(
                    nodeTree,
                    target,
                    event,
                    tunneling,
                    bubbling);
            }

            nodeTree.flushMutationQueue();

            return nodeTree.findNode(targetId) != nullptr;
        }
    }

    InputSystem::InputSystem() = default;

    void InputSystem::rememberNode(
        Node *&node,
        std::optional<Node::Id> &id) noexcept
    {
        setTrackedNode(node, id, node);
    }

    void InputSystem::clearTrackedNode(
        Node *&node,
        std::optional<Node::Id> &id) noexcept
    {
        node = nullptr;
        id.reset();
    }

    void InputSystem::setTrackedNode(
        Node *&node,
        std::optional<Node::Id> &id,
        Node *newNode) noexcept
    {
        node = newNode;

        if (newNode)
        {
            id = newNode->getId();
        }
        else
        {
            id.reset();
        }
    }

    void InputSystem::syncTrackedNode(
        Node *&node,
        std::optional<Node::Id> &id,
        NodeTree &nodeTree,
        bool requireEnabled,
        bool requireFocusable,
        bool requireCapturable)
    {
        if (!id)
        {
            clearTrackedNode(node, id);
            return;
        }

        Node *liveNode = nodeTree.findNode(*id);

        if (!liveNode ||
            !liveNode->isVisible() ||
            (requireEnabled && !liveNode->isEnabled()) ||
            (requireFocusable && !liveNode->isFocusable()) ||
            (requireCapturable && !liveNode->isCapturable()))
        {
            clearTrackedNode(node, id);
            return;
        }

        node = liveNode;
    }

    void InputSystem::processEvent(
        const SDL_Event &sdlEvent,
        NodeTree &nodeTree,
        const Node *modalRoot)
    {
        setModalRootId(modalRoot);

        validateInputState(nodeTree);
        syncState(nodeTree);

        const bool modalIsActive = modalRoot != nullptr;

        switch (sdlEvent.type)
        {
        case SDL_EVENT_MOUSE_MOTION:
        {
            MouseMoveEvent event;

            event.position = {
                static_cast<float>(sdlEvent.motion.x),
                static_cast<float>(sdlEvent.motion.y)};

            input_.pointerPosition_ = event.position;

            Node *node = input_.capturedNode;

            if (!node)
            {
                node = nodeTree.hitTest(
                    event.position.x,
                    event.position.y,
                    modalRoot);
            }

            handleMouseMoveEvent(node, nodeTree, event);
            break;
        }

        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        {
            MouseDownEvent event;

            event.position = {
                static_cast<float>(sdlEvent.button.x),
                static_cast<float>(sdlEvent.button.y)};

            event.button =
                static_cast<MouseButton>(sdlEvent.button.button);

            input_.pointerPosition_ = event.position;

            Node *node = input_.capturedNode;

            if (!node)
            {
                node = nodeTree.hitTest(
                    event.position.x,
                    event.position.y,
                    modalRoot);
            }

            handleMouseDownEvent(
                node,
                nodeTree,
                event,
                modalIsActive);

            break;
        }

        case SDL_EVENT_MOUSE_BUTTON_UP:
        {
            MouseUpEvent event;

            event.position = {
                static_cast<float>(sdlEvent.button.x),
                static_cast<float>(sdlEvent.button.y)};

            event.button =
                static_cast<MouseButton>(sdlEvent.button.button);

            input_.pointerPosition_ = event.position;

            Node *node = input_.capturedNode;

            if (!node)
            {
                node = nodeTree.hitTest(
                    event.position.x,
                    event.position.y,
                    modalRoot);
            }

            handleMouseUpEvent(
                node,
                nodeTree,
                event,
                modalIsActive);

            break;
        }

        case SDL_EVENT_MOUSE_WHEEL:
        {
            MouseWheelEvent event;

            event.position = {
                static_cast<float>(sdlEvent.wheel.mouse_x),
                static_cast<float>(sdlEvent.wheel.mouse_y)};

            event.scrolledX = sdlEvent.wheel.x;
            event.scrolledY = sdlEvent.wheel.y;

            input_.pointerPosition_ = event.position;

            Node *node = input_.capturedNode;

            if (!node)
            {
                node = nodeTree.hitTest(
                    event.position.x,
                    event.position.y,
                    modalRoot);
            }

            handleMouseWheelEvent(node, nodeTree, event);
            break;
        }

        case SDL_EVENT_KEY_DOWN:
        {
            KeyDownEvent event;

            event.is_repeat = sdlEvent.key.repeat;
            event.key = convertSDLKeyCodeToKeyCode(sdlEvent.key.key);

            const SDL_Keymod modifiers = SDL_GetModState();

            event.modifiers.shift = (modifiers & SDL_KMOD_SHIFT) != 0;
            event.modifiers.ctrl  = (modifiers & SDL_KMOD_CTRL) != 0;
            event.modifiers.alt   = (modifiers & SDL_KMOD_ALT) != 0;
            event.modifiers.gui   = (modifiers & SDL_KMOD_GUI) != 0;

            handleKeyDownEvent(nodeTree, event);
            break;
        }

        case SDL_EVENT_KEY_UP:
        {
            KeyUpEvent event;

            event.is_repeat = sdlEvent.key.repeat;
            event.key = convertSDLKeyCodeToKeyCode(sdlEvent.key.key);

            const SDL_Keymod modifiers = SDL_GetModState();

            event.modifiers.shift = (modifiers & SDL_KMOD_SHIFT) != 0;
            event.modifiers.ctrl  = (modifiers & SDL_KMOD_CTRL) != 0;
            event.modifiers.alt   = (modifiers & SDL_KMOD_ALT) != 0;
            event.modifiers.gui   = (modifiers & SDL_KMOD_GUI) != 0;

            handleKeyUpEvent(nodeTree, event);
            break;
        }

        case SDL_EVENT_TEXT_INPUT:
        {
            TextInputEvent event;
            event.text = sdlEvent.text.text ? sdlEvent.text.text : "";
        
            handleTextInputEvent(nodeTree, event);
            break;
        }

        case SDL_EVENT_TEXT_EDITING:
        {
            TextEditingEvent event;
            event.composition = sdlEvent.edit.text ? sdlEvent.edit.text : "";
            event.cursor = sdlEvent.edit.start;
            event.selectionLength = sdlEvent.edit.length;
        
            handleTextEditingEvent(nodeTree, event);
            break;
        }

        default:
            break;
        }

        nodeTree.flushMutationQueue();

        validateInputState(nodeTree);
        syncState(nodeTree);

        nodeTree.flushMutationQueue();

        validateInputState(nodeTree);
        syncState(nodeTree);
    }

    void InputSystem::validateInputState(NodeTree &nodeTree)
    {
        if (input_.focusedNodeId)
        {
            Node *focused = nodeTree.findNode(
                *input_.focusedNodeId);

            if (!focused)
            {
                clearTrackedNode(
                    input_.focusedNode,
                    input_.focusedNodeId);
            }
            else if (!isNodeAllowedByModal(nodeTree, focused) ||
                     !focused->isVisible() ||
                     !focused->isEnabled() ||
                     !focused->isFocusable())
            {
                clearFocus(nodeTree);
            }
            else
            {
                input_.focusedNode = focused;
            }
        }

        if (input_.capturedNodeId)
        {
            Node *captured = nodeTree.findNode(
                *input_.capturedNodeId);

            if (!captured)
            {
                clearTrackedNode(
                    input_.capturedNode,
                    input_.capturedNodeId);

                clearDragState();
            }
            else if (!isNodeAllowedByModal(nodeTree, captured) ||
                     !captured->isVisible() ||
                     !captured->isEnabled() ||
                     !captured->isCapturable())
            {
                cancelPointerInteraction(nodeTree);
            }
            else
            {
                input_.capturedNode = captured;
            }
        }
    }

    void InputSystem::syncState(NodeTree &nodeTree)
    {
        InputState &input = input_;

        syncTrackedNode(
            input.hoveredNode,
            input.hoveredNodeId,
            nodeTree,
            true);

        syncTrackedNode(
            input.focusedNode,
            input.focusedNodeId,
            nodeTree,
            true,
            true);

        syncTrackedNode(
            input.capturedNode,
            input.capturedNodeId,
            nodeTree,
            true,
            false,
            true);

        syncTrackedNode(
            input.pressedNode,
            input.pressedNodeId,
            nodeTree,
            true);

        if (modalRootId_ &&
            nodeTree.findNode(*modalRootId_) == nullptr)
        {
            modalRootId_.reset();
        }

        Node *modalRoot = resolveModalRoot(nodeTree);

        if (modalRoot)
        {
            if (input.hoveredNode &&
                !nodeTree.isDescendant(
                    input.hoveredNode,
                    modalRoot))
            {
                clearTrackedNode(
                    input.hoveredNode,
                    input.hoveredNodeId);
            }

            if (input.focusedNode &&
                !nodeTree.isDescendant(
                    input.focusedNode,
                    modalRoot))
            {
                // Keep the live focus state until validateInputState()
                // performs the semantic FocusLost transition.
            }

            if (input.capturedNode &&
                !nodeTree.isDescendant(
                    input.capturedNode,
                    modalRoot))
            {
                clearTrackedNode(
                    input.capturedNode,
                    input.capturedNodeId);

                clearTrackedNode(
                    input.pressedNode,
                    input.pressedNodeId);

                clearDragState();
            }

            if (input.pressedNode &&
                !nodeTree.isDescendant(
                    input.pressedNode,
                    modalRoot))
            {
                clearTrackedNode(
                    input.pressedNode,
                    input.pressedNodeId);

                input.pressPosition_.reset();
            }
        }

        if (!input.capturedNode)
        {
            clearDragState();
        }

        if (!input.pressedNode)
        {
            input.pressPosition_.reset();
        }
    }

    void InputSystem::resetState()
    {
        input_ = InputState{};
        modalRootId_.reset();

        pendingFocusNodeId_.reset();
        pendingClearFocus_ = false;
        focusTransitionInProgress_ = false;
        dragEndDispatchInProgress_ = false;
    }

    void InputSystem::refreshHover(
        NodeTree &nodeTree,
        float x,
        float y,
        const Node *modalRoot)
    {
        const MousePosition position{x, y};
        input_.pointerPosition_ = position;

        Node *newHovered =
            nodeTree.hitTest(
                position.x,
                position.y,
                modalRoot);

        Node *oldHovered = input_.hoveredNode;

        if (oldHovered == newHovered)
            return;

        if (oldHovered)
        {
            const Node::Id oldHoveredId = oldHovered->getId();

            MouseLeaveEvent leaveEvent;
            leaveEvent.position = position;

            if (!dispatchEvent(
                    nodeTree,
                    oldHovered,
                    leaveEvent,
                    false,
                    false))
            {
                syncState(nodeTree);
                return;
            }

            if (nodeTree.findNode(oldHoveredId) != oldHovered)
            {
                syncState(nodeTree);
                return;
            }
        }

        if (newHovered)
        {
            const Node::Id newHoveredId = newHovered->getId();

            MouseEnterEvent enterEvent;
            enterEvent.position = position;

            if (!dispatchEvent(
                    nodeTree,
                    newHovered,
                    enterEvent,
                    false,
                    false))
            {
                syncState(nodeTree);
                return;
            }

            if (nodeTree.findNode(newHoveredId) != newHovered)
            {
                syncState(nodeTree);
                return;
            }
        }

        setTrackedNode(
            input_.hoveredNode,
            input_.hoveredNodeId,
            newHovered);
    }

    bool InputSystem::focus(
        NodeTree &nodeTree,
        Node &node)
    {
        if (!isNodeAllowedByModal(nodeTree, &node))
            return false;

        if (!node.isVisible() ||
            !node.isEnabled() ||
            !node.isFocusable())
        {
            return false;
        }

        if (nodeTree.findNode(node.getId()) != &node)
        {
            syncState(nodeTree);
            return false;
        }

        const Node::Id requestedId = node.getId();

        if (focusTransitionInProgress_)
        {
            pendingFocusNodeId_ = requestedId;
            pendingClearFocus_ = false;
            return true;
        }

        focusTransitionInProgress_ = true;

        auto finishTransition =
            [this]()
        {
            focusTransitionInProgress_ = false;
        };

        Node *oldFocused = input_.focusedNode;

        if (oldFocused == &node)
        {
            pendingFocusNodeId_.reset();

            FocusGainedEvent event;

            if (!dispatchEvent(
                    nodeTree,
                    &node,
                    event,
                    false,
                    false))
            {
                syncState(nodeTree);
                finishTransition();
                return false;
            }

            syncState(nodeTree);

            const bool success =
                input_.focusedNode == &node;

            finishTransition();

            return success;
        }

        if (oldFocused)
        {
            const Node::Id oldFocusedId = oldFocused->getId();

            FocusLostEvent event;

            // ИСПРАВЛЕНИЕ 1: Сохраняем результат dispatchEvent,
            // а не интерпретируем false как ошибку
            const bool oldFocusedSurvived =
                dispatchEvent(
                    nodeTree,
                    oldFocused,
                    event,
                    false,
                    false);

            if (!oldFocusedSurvived)
            {
                // FocusLost callback может удалить старый узел.
                // Переход фокуса должен продолжаться к запрошенному узлу.
                syncState(nodeTree);
            }

            if (pendingClearFocus_)
            {
                pendingClearFocus_ = false;
                pendingFocusNodeId_.reset();

                clearTrackedNode(
                    input_.focusedNode,
                    input_.focusedNodeId);

                syncState(nodeTree);
                finishTransition();

                return input_.focusedNode == nullptr;
            }

            if (pendingFocusNodeId_)
            {
                const Node::Id pendingId = *pendingFocusNodeId_;
                pendingFocusNodeId_.reset();

                Node *pendingNode = nodeTree.findNode(pendingId);

                if (pendingNode)
                {
                    // The old node has already received FocusLost.
                    // Remove it from the tracked focus state before
                    // starting the pending transition. Otherwise a nested
                    // focus() would dispatch FocusLost on the same node again.
                    if (input_.focusedNode == oldFocused)
                    {
                        clearTrackedNode(
                            input_.focusedNode,
                            input_.focusedNodeId);
                    }

                    focusTransitionInProgress_ = false;
                    return focus(nodeTree, *pendingNode);
                }

                syncState(nodeTree);
                finishTransition();
                return false;
            }

            // ИСПРАВЛЕНИЕ 2: Удаление старого узла - не ошибка,
            // а нормальный сценарий. Продолжаем фокусировку на запрошенном узле.
            if (!nodeTree.findNode(oldFocusedId))
            {
                // Старый узел был удален во время FocusLost.
                // Продолжаем фокусировку на запрошенном узле.
                clearTrackedNode(
                    input_.focusedNode,
                    input_.focusedNodeId);

                syncState(nodeTree);
            }
        }

        setTrackedNode(
            input_.focusedNode,
            input_.focusedNodeId,
            &node);

        FocusGainedEvent event;

        if (!dispatchEvent(
                nodeTree,
                &node,
                event,
                false,
                false))
        {
            syncState(nodeTree);
            finishTransition();
            return false;
        }

        if (pendingClearFocus_)
        {
            pendingClearFocus_ = false;
            pendingFocusNodeId_.reset();

            clearTrackedNode(
                input_.focusedNode,
                input_.focusedNodeId);

            syncState(nodeTree);
            finishTransition();

            return false;
        }

        if (pendingFocusNodeId_)
        {
            const Node::Id pendingId = *pendingFocusNodeId_;
            pendingFocusNodeId_.reset();

            Node *pendingNode = nodeTree.findNode(pendingId);

            if (pendingNode &&
                pendingNode != input_.focusedNode)
            {
                focusTransitionInProgress_ = false;
                return focus(nodeTree, *pendingNode);
            }
        }

        syncState(nodeTree);

        const bool success =
            input_.focusedNode == &node;

        finishTransition();

        return success;
    }

    void InputSystem::clearFocus(NodeTree &nodeTree)
    {
        if (focusTransitionInProgress_)
        {
            pendingFocusNodeId_.reset();
            pendingClearFocus_ = true;
            return;
        }

        Node *oldFocused = input_.focusedNode;

        if (!oldFocused)
            return;

        focusTransitionInProgress_ = true;

        const Node::Id oldFocusedId = oldFocused->getId();

        FocusLostEvent event;

        if (!dispatchEvent(
                nodeTree,
                oldFocused,
                event,
                false,
                false))
        {
            syncState(nodeTree);
            focusTransitionInProgress_ = false;
            return;
        }

        if (pendingClearFocus_)
        {
            pendingClearFocus_ = false;
            pendingFocusNodeId_.reset();

            clearTrackedNode(
                input_.focusedNode,
                input_.focusedNodeId);

            syncState(nodeTree);
            focusTransitionInProgress_ = false;
            return;
        }

        if (pendingFocusNodeId_)
        {
            const Node::Id pendingId = *pendingFocusNodeId_;
            pendingFocusNodeId_.reset();

            Node *pendingNode = nodeTree.findNode(pendingId);

            if (pendingNode)
            {
                focusTransitionInProgress_ = false;
                focus(nodeTree, *pendingNode);
                return;
            }

            syncState(nodeTree);
            focusTransitionInProgress_ = false;
            return;
        }

        if (!nodeTree.findNode(oldFocusedId))
        {
            syncState(nodeTree);
            focusTransitionInProgress_ = false;
            return;
        }

        if (input_.focusedNode == oldFocused)
        {
            clearTrackedNode(
                input_.focusedNode,
                input_.focusedNodeId);
        }

        syncState(nodeTree);

        focusTransitionInProgress_ = false;
    }

    bool InputSystem::capture(
        NodeTree &nodeTree,
        Node &node,
        std::optional<MousePosition> pressPosition)
    {
        Node *target = &node;

        if (!isNodeAllowedByModal(nodeTree, target))
            return false;

        if (!target->isVisible() ||
            !target->isEnabled() ||
            !target->isCapturable())
        {
            return false;
        }

        if (nodeTree.findNode(target->getId()) != target)
        {
            syncState(nodeTree);
            return false;
        }

        if (input_.capturedNode)
        {
            const Node::Id previousCaptureId =
                input_.capturedNode->getId();

            cancelPointerInteraction(nodeTree);

            if (input_.capturedNode &&
                input_.capturedNode->getId() != previousCaptureId)
            {
                syncState(nodeTree);
                return true;
            }

            target = nodeTree.findNode(target->getId());

            if (!target)
                return false;
        }

        setTrackedNode(
            input_.capturedNode,
            input_.capturedNodeId,
            target);

        setTrackedNode(
            input_.pressedNode,
            input_.pressedNodeId,
            target);

        input_.isDragging = false;
        input_.pressPosition_ = std::move(pressPosition);

        return true;
    }

    void InputSystem::releaseCapture(
        NodeTree &nodeTree,
        std::optional<MousePosition> position)
    {
        Node *captured = input_.capturedNode;

        if (!captured)
            return;

        const Node::Id capturedId = captured->getId();

        if (!dispatchDragEndIfNeeded(
                nodeTree,
                captured,
                std::move(position)))
        {
            return;
        }

        // DragEndEvent may have changed pointer capture.
        // Never overwrite a callback-established state.
        if (input_.capturedNode &&
            input_.capturedNode->getId() != capturedId)
        {
            syncState(nodeTree);
            return;
        }

        if (!input_.capturedNode)
        {
            syncState(nodeTree);
            return;
        }

        clearTrackedNode(
            input_.capturedNode,
            input_.capturedNodeId);

        clearTrackedNode(
            input_.pressedNode,
            input_.pressedNodeId);

        clearDragState();
    }

    void InputSystem::cancelPointerInteraction(
        NodeTree &nodeTree,
        std::optional<MousePosition> position)
    {
        Node *captured = input_.capturedNode;

        const std::optional<Node::Id> capturedId =
            captured
                ? std::optional<Node::Id>(captured->getId())
                : std::nullopt;

        if (captured &&
            !dispatchDragEndIfNeeded(
                nodeTree,
                captured,
                position))
        {
            return;
        }

        // DragEndEvent may have changed pointer capture.
        // Never overwrite callback-established capture state.
        if (capturedId &&
            input_.capturedNode &&
            input_.capturedNode->getId() != *capturedId)
        {
            syncState(nodeTree);
            return;
        }

        Node *hovered = input_.hoveredNode;

        const std::optional<Node::Id> hoveredId =
            hovered
                ? std::optional<Node::Id>(hovered->getId())
                : std::nullopt;

        if (!dispatchMouseLeaveIfNeeded(
                nodeTree,
                hovered,
                position))
        {
            return;
        }

        // MouseLeaveEvent may mutate pointer state:
        // - establish a new capture;
        // - establish a new pressed node;
        // - replace the hovered node.
        //
        // Never clear callback-established state.
        if (capturedId &&
            input_.capturedNode &&
            input_.capturedNode->getId() != *capturedId)
        {
            syncState(nodeTree);
            return;
        }

        if (!capturedId &&
            input_.capturedNode)
        {
            syncState(nodeTree);
            return;
        }

        if (hoveredId &&
            input_.hoveredNode &&
            input_.hoveredNode->getId() != *hoveredId)
        {
            syncState(nodeTree);
            return;
        }

        clearPointerTracking();
    }

    void InputSystem::setModalRoot(const Node *modalRoot) noexcept
    {
        setModalRootId(modalRoot);
    }

    Node *InputSystem::focusedNode() const noexcept
    {
        return input_.focusedNode;
    }

    std::optional<Node::Id> InputSystem::focusedNodeId() const noexcept
    {
        return input_.focusedNodeId;
    }

    Node *InputSystem::capturedNode() const noexcept
    {
        return input_.capturedNode;
    }

    Node *InputSystem::pressedNode() const noexcept
    {
        return input_.pressedNode;
    }

    Node *InputSystem::hoveredNode() const noexcept
    {
        return input_.hoveredNode;
    }

    bool InputSystem::isDragging() const noexcept
    {
        return input_.isDragging;
    }

    void InputSystem::setModalRootId(
        const Node *modalRoot) noexcept
    {
        modalRootId_ =
            modalRoot ? std::optional<Node::Id>{modalRoot->getId()} : std::nullopt;
    }

    bool InputSystem::dispatchDragEndIfNeeded(
        NodeTree &nodeTree,
        Node *node,
        std::optional<MousePosition> position)
    {
        if (!node || !input_.isDragging)
            return true;

        if (dragEndDispatchInProgress_)
            return true;

        const Node::Id nodeId = node->getId();

        MousePosition pos = position.value_or(MousePosition{});

        MouseDragEndEvent event;
        event.position = pos;

        if (input_.pressPosition_)
        {
            const MousePosition pressed = *input_.pressPosition_;

            event.delta = {
                pos.x - pressed.x,
                pos.y - pressed.y};
        }
        else
        {
            event.delta = {};
        }

        dragEndDispatchInProgress_ = true;

        const bool dispatched =
            dispatchEvent(
                nodeTree,
                node,
                event,
                false,
                false);

        dragEndDispatchInProgress_ = false;

        if (!dispatched)
        {
            syncState(nodeTree);
            return false;
        }

        if (!nodeTree.findNode(nodeId))
        {
            syncState(nodeTree);
            return false;
        }

        return true;
    }

    bool InputSystem::dispatchMouseLeaveIfNeeded(
        NodeTree &nodeTree,
        Node *node,
        std::optional<MousePosition> position)
    {
        if (!node)
            return true;

        const Node::Id nodeId = node->getId();

        MouseLeaveEvent leaveEvent;
        leaveEvent.position = position.value_or(MousePosition{});

        if (!dispatchEvent(
                nodeTree,
                node,
                leaveEvent,
                false,
                false))
        {
            syncState(nodeTree);
            return false;
        }

        if (!nodeTree.findNode(nodeId))
        {
            syncState(nodeTree);
            return false;
        }

        return true;
    }

    void InputSystem::clearDragState() noexcept
    {
        input_.isDragging = false;
        input_.pressPosition_.reset();
    }

    void InputSystem::clearPointerTracking() noexcept
    {
        clearTrackedNode(
            input_.hoveredNode,
            input_.hoveredNodeId);

        clearTrackedNode(
            input_.capturedNode,
            input_.capturedNodeId);

        clearTrackedNode(
            input_.pressedNode,
            input_.pressedNodeId);

        clearDragState();
    }

    Node *InputSystem::resolveModalRoot(NodeTree &nodeTree) const noexcept
    {
        if (!modalRootId_)
            return nullptr;

        return nodeTree.findNode(*modalRootId_);
    }

    bool InputSystem::isNodeAllowedByModal(
        NodeTree &nodeTree,
        Node *node) const noexcept
    {
        if (!node)
            return false;

        if (!modalRootId_)
            return true;

        Node *modalRoot =
            nodeTree.findNode(*modalRootId_);

        if (!modalRoot)
            return false;

        if (nodeTree.findNode(node->getId()) != node)
            return false;

        return nodeTree.isDescendant(
            node,
            modalRoot);
    }

    void InputSystem::handleMouseMoveEvent(
        Node *node,
        NodeTree &nodeTree,
        MouseMoveEvent &event)
    {
        input_.pointerPosition_ = event.position;

        InputState &input = input_;

        if (!input.capturedNode)
        {
            if (input.hoveredNode != node)
            {
                if (input.hoveredNode)
                {
                    Node *oldHovered = input.hoveredNode;
                    const Node::Id oldHoveredId = oldHovered->getId();

                    MouseLeaveEvent leaveEvent;
                    leaveEvent.position = event.position;

                    if (!dispatchEvent(
                            nodeTree,
                            oldHovered,
                            leaveEvent,
                            false,
                            false))
                    {
                        syncState(nodeTree);
                        return;
                    }

                    if (!nodeTree.findNode(oldHoveredId))
                    {
                        syncState(nodeTree);
                        return;
                    }
                }

                if (node)
                {
                    const Node::Id enterId = node->getId();

                    MouseEnterEvent enterEvent;
                    enterEvent.position = event.position;

                    if (!dispatchEvent(
                            nodeTree,
                            node,
                            enterEvent,
                            false,
                            false))
                    {
                        syncState(nodeTree);
                        return;
                    }

                    if (nodeTree.findNode(enterId) != node)
                    {
                        syncState(nodeTree);
                        return;
                    }
                }

                setTrackedNode(
                    input.hoveredNode,
                    input.hoveredNodeId,
                    node);
            }

            clearDragState();
        }

        Node *dispatchTarget =
            input.capturedNode ? input.capturedNode : node;

        if (!dispatchTarget)
            return;

        if (input.capturedNode)
        {
            if (!input.pressPosition_)
            {
                input.pressPosition_ = event.position;
            }

            const auto [mouseX, mouseY] = event.position;
            const auto [pressedX, pressedY] = *input.pressPosition_;

            const float dx = mouseX - pressedX;
            const float dy = mouseY - pressedY;

            const float distanceSquared = dx * dx + dy * dy;
            const float thresholdSquared =
                input.dragThreshold * input.dragThreshold;

            if (!input.isDragging &&
                distanceSquared > thresholdSquared)
            {
                input.isDragging = true;

                MouseDragBeginEvent beginEvent;
                beginEvent.dragging = true;
                beginEvent.position = event.position;
                beginEvent.delta = {dx, dy};

                Node *captured = input.capturedNode;
                const Node::Id capturedId = captured->getId();

                if (!dispatchEvent(
                        nodeTree,
                        captured,
                        beginEvent,
                        false,
                        false))
                {
                    syncState(nodeTree);
                    return;
                }

                if (!nodeTree.findNode(capturedId))
                {
                    syncState(nodeTree);
                    return;
                }

                if (input.isDragging && input.capturedNode)
                {
                    MouseDragEvent dragEvent;
                    dragEvent.dragging = true;
                    dragEvent.position = event.position;
                    dragEvent.delta = {dx, dy};

                    Node *capturedNow = input.capturedNode;
                    const Node::Id capturedNowId = capturedNow->getId();

                    if (!dispatchEvent(
                            nodeTree,
                            capturedNow,
                            dragEvent,
                            false,
                            false))
                    {
                        syncState(nodeTree);
                        return;
                    }

                    if (nodeTree.findNode(capturedNowId) != capturedNow)
                    {
                        syncState(nodeTree);
                        return;
                    }
                }
            }
        }

        Node *currentTarget =
            input.capturedNode ? input.capturedNode : node;

        if (!currentTarget)
            return;

        const Node::Id dispatchId = currentTarget->getId();

        if (!dispatchEvent(
                nodeTree,
                currentTarget,
                event,
                false,
                false))
        {
            syncState(nodeTree);
            return;
        }

        if (nodeTree.findNode(dispatchId) != currentTarget)
        {
            syncState(nodeTree);
        }
    }

    void InputSystem::handleMouseDownEvent(
        Node *node,
        NodeTree &nodeTree,
        MouseDownEvent &event,
        bool modalIsActive)
    {
        input_.pointerPosition_ = event.position;

        InputState &input = input_;

        if (!node)
        {
            cancelPointerInteraction(nodeTree);

            if (!modalIsActive)
            {
                clearFocus(nodeTree);
            }

            return;
        }

        setTrackedNode(
            input.pressedNode,
            input.pressedNodeId,
            node);

        const bool hadCaptureBefore = input.capturedNode != nullptr;
        const bool hadFocusBefore = input.focusedNode != nullptr;

        const Node::Id nodeId = node->getId();

        if (!dispatchEvent(
                nodeTree,
                node,
                event,
                true,
                true))
        {
            syncState(nodeTree);
            return;
        }

        Node *liveNode = nodeTree.findNode(nodeId);

        if (!liveNode)
        {
            syncState(nodeTree);
            return;
        }

        if (!hadCaptureBefore &&
            !input.capturedNode &&
            liveNode->isCapturable())
        {
            capture(nodeTree, *liveNode, event.position);
        }

        if (liveNode->isFocusable())
        {
            if (!focus(nodeTree, *liveNode))
            {
                syncState(nodeTree);
                return;
            }

            liveNode = nodeTree.findNode(liveNode->getId());

            if (!liveNode)
            {
                syncState(nodeTree);
                return;
            }
        }

        syncState(nodeTree);
    }

    void InputSystem::handleMouseUpEvent(
        Node *node,
        NodeTree &nodeTree,
        MouseUpEvent &event,
        bool modalIsActive)
    {
        input_.pointerPosition_ = event.position;

        InputState &input = input_;

        const std::optional<Node::Id> initialCaptureId =
            input.capturedNode
                ? std::optional<Node::Id>(input.capturedNode->getId())
                : std::nullopt;

        const std::optional<Node::Id> initialPressedId =
            input.pressedNode
                ? std::optional<Node::Id>(input.pressedNode->getId())
                : std::nullopt;

        Node *releaseNode =
            input.capturedNode ? input.capturedNode : node;

        const Node::Id underNodeId =
            node ? node->getId() : 0;

        if (!releaseNode)
        {
            cancelPointerInteraction(nodeTree);

            if (!modalIsActive)
            {
                clearFocus(nodeTree);
            }

            return;
        }

        const Node::Id releaseId = releaseNode->getId();

        if (!dispatchEvent(
                nodeTree,
                releaseNode,
                event,
                true,
                true))
        {
            syncState(nodeTree);
            return;
        }

        Node *liveReleaseNode = nodeTree.findNode(releaseId);

        if (!liveReleaseNode)
        {
            syncState(nodeTree);
            return;
        }

        const std::optional<Node::Id> captureAfterMouseUp =
            input.capturedNode
                ? std::optional<Node::Id>(input.capturedNode->getId())
                : std::nullopt;

        if (captureAfterMouseUp != initialCaptureId)
        {
            syncState(nodeTree);
            return;
        }

        Node *pressedNode = input.pressedNode;

        if (initialPressedId)
        {
            if (!pressedNode ||
                pressedNode->getId() != *initialPressedId)
            {
                syncState(nodeTree);
                return;
            }
        }

        if (pressedNode)
        {
            const Node::Id pressedId = pressedNode->getId();

            Node *livePressedNode = nodeTree.findNode(pressedId);

            if (!livePressedNode)
            {
                syncState(nodeTree);
                return;
            }

            Node *liveUnderNode = underNodeId ? nodeTree.findNode(underNodeId) : nullptr;

            const std::optional<Node::Id> captureBeforeClick =
                input.capturedNode
                    ? std::optional<Node::Id>(input.capturedNode->getId())
                    : std::nullopt;

            if (!input.isDragging &&
                livePressedNode == liveReleaseNode &&
                liveUnderNode &&
                underNodeId == releaseId)
            {
                MouseClickEvent clickEvent;
                clickEvent.position = event.position;
                clickEvent.button = event.button;

                if (!dispatchEvent(
                        nodeTree,
                        liveReleaseNode,
                        clickEvent,
                        false,
                        true))
                {
                    syncState(nodeTree);
                    return;
                }

                const std::optional<Node::Id> captureAfterClick =
                    input.capturedNode
                        ? std::optional<Node::Id>(input.capturedNode->getId())
                        : std::nullopt;

                if (captureAfterClick != captureBeforeClick)
                {
                    syncState(nodeTree);
                    return;
                }

                if (nodeTree.findNode(releaseId) != liveReleaseNode)
                {
                    syncState(nodeTree);
                    return;
                }
            }
        }

        releaseCapture(nodeTree, event.position);
    }

    void InputSystem::handleMouseWheelEvent(
        Node *node,
        NodeTree &nodeTree,
        MouseWheelEvent &event)
    {
        input_.pointerPosition_ = event.position;

        if (!node)
            return;

        const Node::Id nodeId = node->getId();

        if (!dispatchEvent(
                nodeTree,
                node,
                event,
                false,
                true))
        {
            syncState(nodeTree);
            return;
        }

        if (nodeTree.findNode(nodeId) != node)
        {
            syncState(nodeTree);
        }
    }

    void InputSystem::handleKeyDownEvent(
        NodeTree &nodeTree,
        KeyDownEvent &event)
    {
        Node *target = input_.focusedNode;

        if (!target)
            return;

        const Node::Id targetId = target->getId();

        if (!dispatchEvent(
                nodeTree,
                target,
                event,
                true,
                true))
        {
            syncState(nodeTree);
            return;
        }

        if (nodeTree.findNode(targetId) != target)
        {
            syncState(nodeTree);
        }
    }

    void InputSystem::handleKeyUpEvent(
        NodeTree &nodeTree,
        KeyUpEvent &event)
    {
        Node *target = input_.focusedNode;

        if (!target)
            return;

        const Node::Id targetId = target->getId();

        if (!dispatchEvent(
                nodeTree,
                target,
                event,
                true,
                true))
        {
            syncState(nodeTree);
            return;
        }

        if (nodeTree.findNode(targetId) != target)
        {
            syncState(nodeTree);
        }
    }

    void InputSystem::handleTextInputEvent(
        NodeTree &nodeTree,
        TextInputEvent &event)
    {
        Node *focused = input_.focusedNode;
        if (!focused)
            return;
    
        dispatchEvent(
            nodeTree,
            focused,
            event,
            true,
            true);
    }

    void InputSystem::handleTextEditingEvent(
        NodeTree &nodeTree,
        TextEditingEvent &event)
    {
        Node *focused = input_.focusedNode;
        if (!focused)
            return;
    
        dispatchEvent(
            nodeTree,
            focused,
            event,
            true,
            true);
    }

    KeyCode InputSystem::convertSDLKeyCodeToKeyCode(
        SDL_Keycode sdlKey) const
    {
#if defined(SDLK_a)
        if (sdlKey >= SDLK_A && sdlKey <= SDLK_Z)
        {
            return static_cast<KeyCode>(
                static_cast<int>(KeyCode::A) + (sdlKey - SDLK_A));
        }
#elif defined(SDLK_A)
        if (sdlKey >= SDLK_A && sdlKey <= SDLK_Z)
        {
            return static_cast<KeyCode>(
                static_cast<int>(KeyCode::A) + (sdlKey - SDLK_A));
        }
#endif

#if defined(SDLK_0) && defined(SDLK_9)
        if (sdlKey >= SDLK_0 && sdlKey <= SDLK_9)
        {
            return static_cast<KeyCode>(
                static_cast<int>(KeyCode::NUM_0) + (sdlKey - SDLK_0));
        }
#endif

#if defined(SDLK_F1) && defined(SDLK_F24)
        if (sdlKey >= SDLK_F1 && sdlKey <= SDLK_F24)
        {
            return static_cast<KeyCode>(
                static_cast<int>(KeyCode::F1) + (sdlKey - SDLK_F1));
        }
#endif

        switch (sdlKey)
        {
        case SDLK_SPACE:
            return KeyCode::SPACE;

        case SDLK_RETURN:
            return KeyCode::ENTER;

        case SDLK_ESCAPE:
            return KeyCode::ESCAPE;

        case SDLK_TAB:
            return KeyCode::TAB;

        case SDLK_BACKSPACE:
            return KeyCode::BACKSPACE;

        case SDLK_DELETE:
            return KeyCode::DELETE;

        case SDLK_INSERT:
            return KeyCode::INSERT;

        case SDLK_HOME:
            return KeyCode::HOME;

        case SDLK_END:
            return KeyCode::END;

        case SDLK_PAGEUP:
            return KeyCode::PAGE_UP;

        case SDLK_PAGEDOWN:
            return KeyCode::PAGE_DOWN;

        case SDLK_UP:
            return KeyCode::UP;

        case SDLK_DOWN:
            return KeyCode::DOWN;

        case SDLK_LEFT:
            return KeyCode::LEFT;

        case SDLK_RIGHT:
            return KeyCode::RIGHT;

        case SDLK_LSHIFT:
            return KeyCode::LSHIFT;

        case SDLK_RSHIFT:
            return KeyCode::RSHIFT;

        case SDLK_LCTRL:
            return KeyCode::LCTRL;

        case SDLK_RCTRL:
            return KeyCode::RCTRL;

        case SDLK_LALT:
            return KeyCode::LALT;

        case SDLK_RALT:
            return KeyCode::RALT;

        case SDLK_LGUI:
            return KeyCode::LGUI;

        case SDLK_RGUI:
            return KeyCode::RGUI;

        case SDLK_CAPSLOCK:
            return KeyCode::CAPS_LOCK;

        case SDLK_NUMLOCKCLEAR:
            return KeyCode::NUM_LOCK;

        case SDLK_SCROLLLOCK:
            return KeyCode::SCROLL_LOCK;

        case SDLK_PAUSE:
            return KeyCode::PAUSE;

        case SDLK_PRINTSCREEN:
            return KeyCode::PRINT_SCREEN;

        case SDLK_COMMA:
            return KeyCode::COMMA;

        case SDLK_PERIOD:
            return KeyCode::PERIOD;

        case SDLK_SLASH:
            return KeyCode::SLASH;

        case SDLK_SEMICOLON:
            return KeyCode::SEMICOLON;

        case SDLK_APOSTROPHE:
            return KeyCode::QUOTE;

        case SDLK_LEFTBRACKET:
            return KeyCode::LBRACKET;

        case SDLK_RIGHTBRACKET:
            return KeyCode::RBRACKET;

        case SDLK_BACKSLASH:
            return KeyCode::BACKSLASH;

        case SDLK_GRAVE:
            return KeyCode::GRAVE;

        case SDLK_MINUS:
            return KeyCode::MINUS;

        case SDLK_EQUALS:
            return KeyCode::EQUALS;

        default:
            return KeyCode::UNKNOWN;
        }
    }

}
