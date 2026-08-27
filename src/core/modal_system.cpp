#include "modal_system.hpp"
#include "node_tree.hpp"
#include "input_system.hpp"
#include "ui_framework/panel_node.hpp"

#include <SDL3/SDL.h>

#include <algorithm>
#include <cstddef>
#include <memory>
#include <optional>

namespace ui
{
    class ModalSystem::BackdropNode final : public Node
    {
    public:
        BackdropNode() = default;

        void setBackdrop(const Color &color, float opacity) noexcept
        {
            color_ = color;
            opacity_ = std::clamp(opacity, 0.0f, 1.0f);
        }

        LayoutSize measureContent(const LayoutSize &) const override
        {
            return getActualSize();
        }

        void arrangeContent(const LayoutPosition &, const LayoutSize &) override
        {
        }

        Node *hitTest(float, float) noexcept override
        {
            return nullptr;
        }

    protected:
        void draw(SDL_Renderer *renderer) override
        {
            if (!renderer || opacity_ <= 0.0f)
                return;

            SDL_Color previousColor{};
            Uint8 previousAlpha = 255;
            SDL_BlendMode previousBlendMode = SDL_BLENDMODE_NONE;
            SDL_GetRenderDrawColor(renderer, &previousColor.r, &previousColor.g, &previousColor.b, &previousAlpha);
            SDL_GetRenderDrawBlendMode(renderer, &previousBlendMode);

            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(
                renderer,
                color_.r,
                color_.g,
                color_.b,
                static_cast<Uint8>(std::clamp(color_.a * opacity_, 0.0f, 255.0f)));

            const LayoutPosition position = getActualPosition();
            const LayoutSize size = getActualSize();
            SDL_FRect rect{position.x, position.y, size.width, size.height};
            SDL_RenderFillRect(renderer, &rect);

            SDL_SetRenderDrawColor(
                renderer,
                previousColor.r,
                previousColor.g,
                previousColor.b,
                previousAlpha);
            SDL_SetRenderDrawBlendMode(renderer, previousBlendMode);
        }

    private:
        Color color_{0, 0, 0, 160};
        float opacity_ = 0.0f;
    };

    ModalSystem::ModalSystem() = default;

    bool ModalSystem::showModal(NodeTree &nodeTree, InputSystem &input, Node &node)
    {
        return showModal(nodeTree, input, node, ModalOptions{});
    }

    bool ModalSystem::showModal(NodeTree &nodeTree, InputSystem &input, Node &node, BackdropClickBehavior backdropClickBehavior)
    {
        ModalOptions options;
        options.outsideClick = backdropClickBehavior;
        return showModal(nodeTree, input, node, options);
    }

    bool ModalSystem::showModal(NodeTree &nodeTree, InputSystem &input, Node &node, const ModalOptions &options)
    {
        if (!nodeTree.isNodeLive(node.getId()) ||
            !nodeTree.isOverlay(&node) ||
            !node.isVisible() ||
            !node.isEnabled() ||
            isModal(&node))
            return false;

        const Node::Id modalId = node.getId();
        const std::optional<Node::Id> previousFocusId = input.focusedNodeId();
        const std::optional<Node::Id> previousModalId =
            modals_.empty() ? std::nullopt : std::optional<Node::Id>(modals_.back().modalId);

        input.cancelPointerInteraction(nodeTree);

        Node *liveModal = nodeTree.findNode(modalId);
        if (!liveModal || !liveModal->isVisible() || !liveModal->isEnabled())
        {
            input.syncState(nodeTree);
            return false;
        }

        modals_.push_back({modalId, previousFocusId, previousModalId, options});
        input.setModalRoot(liveModal);

        if (options.showBackdrop)
            ensureBackdrop(nodeTree);

        startBackdropAnimation(nodeTree);

        if (liveModal->isFocusable())
        {
            if (!input.focus(nodeTree, *liveModal))
                focusOrClear(nodeTree, input, findFirstFocusable(*liveModal));
        }
        else
        {
            focusOrClear(nodeTree, input, findFirstFocusable(*liveModal));
        }

        input.syncState(nodeTree);
        return true;
    }

    bool ModalSystem::closeModal(NodeTree &nodeTree, InputSystem &input)
    {
        if (modals_.empty())
            return false;

        const ModalSession session = modals_.back();
        modals_.pop_back();

        input.cancelPointerInteraction(nodeTree);
        input.clearFocus(nodeTree);

        if (Node *nextModal = topModalNode(nodeTree))
            input.setModalRoot(nextModal);
        else
            input.setModalRoot(nullptr);

        restoreFocusAfterClose(nodeTree, input, session);
        startBackdropAnimation(nodeTree);
        input.syncState(nodeTree);
        return true;
    }

    bool ModalSystem::handleKeyDown(NodeTree &nodeTree, InputSystem &input, KeyCode key)
    {
        return handleKeyEvent(nodeTree, input, key, false);
    }

    bool ModalSystem::handleKeyEvent(NodeTree &nodeTree, InputSystem &input, KeyCode key, bool propagationStopped)
    {
        if (modals_.empty() || propagationStopped)
            return false;

        Node *topModal = topModalNode(nodeTree);
        if (!topModal)
        {
            sync(nodeTree, input);
            return false;
        }

        if (key == KeyCode::ESCAPE)
        {
            if (!modals_.back().options.closeOnEscape)
                return false;
            return closeModal(nodeTree, input);
        }

        if (key == KeyCode::TAB)
        {
            Node *focused = input.focusedNode();
            if (!focused || !isNodeUnder(focused, topModal))
            {
                focusOrClear(nodeTree, input, findFirstFocusable(*topModal));
                return true;
            }

            Node *next = findNextFocusableInModal(nodeTree, *topModal, *focused);
            if (!next)
                next = findFirstFocusable(*topModal);

            if (next)
            {
                input.focus(nodeTree, *next);
                return true;
            }

            return true;
        }

        return false;
    }

    bool ModalSystem::handlePointerDown(NodeTree &nodeTree, InputSystem &input, const MousePosition &position, MouseButton)
    {
        if (modals_.empty())
            return false;

        Node *modalRoot = topModalNode(nodeTree);
        if (!modalRoot)
        {
            sync(nodeTree, input);
            return false;
        }

        if (nodeTree.hitTest(position.x, position.y, modalRoot))
            return false;

        const OutsideClickBehavior behavior = modals_.back().options.outsideClick;
        input.cancelPointerInteraction(nodeTree, position);

        if (behavior == OutsideClickBehavior::Close)
            closeModal(nodeTree, input);

        return true;
    }

    bool ModalSystem::isModal(const Node *node) const noexcept
    {
        if (!node)
            return false;
        const Node::Id id = node->getId();
        return std::any_of(modals_.begin(), modals_.end(),
            [id](const ModalSession &session) { return session.modalId == id; });
    }

    Node *ModalSystem::topModalNode(NodeTree &nodeTree) const noexcept
    {
        return modals_.empty() ? nullptr : nodeTree.findNode(modals_.back().modalId);
    }

    const Node *ModalSystem::topModalNode(const NodeTree &nodeTree) const noexcept
    {
        return modals_.empty() ? nullptr : nodeTree.findNode(modals_.back().modalId);
    }

    Node *ModalSystem::backdropNode(NodeTree &nodeTree) const noexcept
    {
        if (!backdropId_)
            return nullptr;
        return nodeTree.findNode(*backdropId_);
    }

    const Node *ModalSystem::backdropNode(const NodeTree &nodeTree) const noexcept
    {
        if (!backdropId_)
            return nullptr;
        return nodeTree.findNode(*backdropId_);
    }

    void ModalSystem::setViewportSize(const LayoutSize &size) noexcept
    {
        viewportSize_.width = std::max(0.0f, size.width);
        viewportSize_.height = std::max(0.0f, size.height);

        if (backdropNode_)
        {
            backdropNode_->setPosition({0.0f, 0.0f});
            backdropNode_->setSize(LayoutSizeValue::fixed(viewportSize_.width, viewportSize_.height));
        }
    }

    void ModalSystem::setBackdropColor(const Color &color) noexcept
    {
        backdropColor_ = color;
        if (backdropNode_)
            backdropNode_->setBackdrop(backdropColor_, backdropOpacity_);
    }

    Color ModalSystem::getBackdropColor() const noexcept
    {
        return backdropColor_;
    }

    void ModalSystem::setBackdropFadeDuration(float seconds) noexcept
    {
        backdropFadeDuration_ = std::max(0.0f, seconds);
    }

    float ModalSystem::getBackdropFadeDuration() const noexcept
    {
        return backdropFadeDuration_;
    }

    void ModalSystem::clear(NodeTree &nodeTree, InputSystem &input) noexcept
    {
        modals_.clear();
        input.cancelPointerInteraction(nodeTree);
        input.clearFocus(nodeTree);
        input.setModalRoot(nullptr);

        if (backdropNode_)
            backdropNode_->cancelAnimation(&backdropOpacity_);

        removeBackdrop(nodeTree);
        backdropOpacity_ = 0.0f;
        backdropTargetOpacity_ = 0.0f;
    }

    void ModalSystem::sync(NodeTree &nodeTree, InputSystem &input)
    {
        for (size_t i = modals_.size(); i > 0; --i)
        {
            if (eraseInvalidModalSession(nodeTree, input, i - 1))
                break;
        }

        updateBackdropState();

        if (backdropTargetOpacity_ > 0.0f && !backdropNode_)
        {
            ensureBackdrop(nodeTree);
            if (backdropNode_)
                startBackdropAnimation(nodeTree);
        }

        if (backdropNode_)
        {
            backdropNode_->setBackdrop(backdropColor_, backdropOpacity_);
            backdropNode_->setPosition({0.0f, 0.0f});
            backdropNode_->setSize(LayoutSizeValue::fixed(viewportSize_.width, viewportSize_.height));

            if (backdropTargetOpacity_ <= 0.0f && backdropOpacity_ <= 0.0f)
                removeBackdrop(nodeTree);
        }

        if (Node *topModal = topModalNode(nodeTree))
        {
            input.setModalRoot(topModal);
            syncFocusForTopModal(nodeTree, input);
        }
        else
        {
            input.setModalRoot(nullptr);
        }
    }

    Node *ModalSystem::findFirstFocusable(Node &node) const
    {
        if (!node.isVisible() || !node.isEnabled())
            return nullptr;
        if (node.isFocusable())
            return &node;

        auto *panel = dynamic_cast<PanelNode *>(&node);
        if (!panel)
            return nullptr;

        Node *result = nullptr;
        panel->forEachChild([this, &result](Node &child)
        {
            result = findFirstFocusable(child);
            return result != nullptr;
        });
        return result;
    }

    bool ModalSystem::collectFocusable(Node &node, const Node *, std::vector<Node *> &nodes) const
    {
        if (!node.isVisible() || !node.isEnabled())
            return true;

        if (node.isFocusable())
            nodes.push_back(&node);

        auto *panel = dynamic_cast<PanelNode *>(&node);
        if (!panel)
            return true;

        panel->forEachChild([this, &nodes](Node &child)
        {
            collectFocusable(child, nullptr, nodes);
            return false;
        });
        return true;
    }

    Node *ModalSystem::findNextFocusableInModal(NodeTree &, Node &modal, const Node &current) const
    {
        std::vector<Node *> focusables;
        focusables.reserve(16);
        collectFocusable(modal, &current, focusables);

        if (focusables.empty())
            return nullptr;

        const auto it = std::find(focusables.begin(), focusables.end(), &current);
        if (it == focusables.end())
            return focusables.front();

        const std::size_t index = static_cast<std::size_t>(std::distance(focusables.begin(), it));
        return focusables[(index + 1) % focusables.size()];
    }

    Node *ModalSystem::findFirstFocusableInTree(NodeTree &nodeTree) const
    {
        Node *result = nullptr;
        nodeTree.forEachRoot([this, &result](Node &root)
        {
            result = findFirstFocusable(root);
            return result == nullptr;
        });
        if (!result)
            nodeTree.forEachOverlay([this, &result](Node &overlay)
            {
                result = findFirstFocusable(overlay);
                return result == nullptr;
            });
        return result;
    }

    Node *ModalSystem::findFirstFocusableInModal(NodeTree &nodeTree, std::optional<Node::Id> modalId) const
    {
        if (!modalId)
            return nullptr;
        Node *modal = nodeTree.findNode(*modalId);
        if (!modal || !nodeTree.isOverlay(modal) || !modal->isVisible() || !modal->isEnabled())
            return nullptr;
        return findFirstFocusable(*modal);
    }

    Node *ModalSystem::findValidFocus(NodeTree &nodeTree, std::optional<Node::Id> preferredFocusId) const
    {
        if (preferredFocusId)
        {
            Node *preferred = nodeTree.findNode(*preferredFocusId);
            if (preferred && preferred->isVisible() && preferred->isEnabled() && preferred->isFocusable())
                return preferred;
        }
        return findFirstFocusableInTree(nodeTree);
    }

    bool ModalSystem::isNodeUnder(const Node *node, const Node *ancestor) const noexcept
    {
        for (const Node *current = node; current; current = current->getParent())
            if (current == ancestor)
                return true;
        return false;
    }

    void ModalSystem::restoreFocusAfterClose(NodeTree &nodeTree, InputSystem &input, const ModalSession &session) const
    {
        if (Node *previousModalFocus = findFirstFocusableInModal(nodeTree, session.previousModalId))
        {
            focusOrClear(nodeTree, input, previousModalFocus);
            return;
        }
        focusOrClear(nodeTree, input, findValidFocus(nodeTree, session.previousFocusId));
    }

    void ModalSystem::syncFocusForTopModal(NodeTree &nodeTree, InputSystem &input) const
    {
        if (modals_.empty())
            return;

        Node *topModal = nodeTree.findNode(modals_.back().modalId);
        if (!topModal || !topModal->isVisible())
            return;

        if (!topModal->isEnabled())
        {
            if (input.focusedNode() && isNodeUnder(input.focusedNode(), topModal))
                input.clearFocus(nodeTree);
            return;
        }

        if (!input.focusedNode())
        {
            focusOrClear(nodeTree, input, findFirstFocusable(*topModal));
            return;
        }

        if (!isNodeUnder(input.focusedNode(), topModal))
            focusOrClear(nodeTree, input, findFirstFocusable(*topModal));
    }

    bool ModalSystem::isLiveVisibleEnabledModal(NodeTree &nodeTree, const Node &node) const noexcept
    {
        const Node *liveNode = nodeTree.findNode(node.getId());
        return liveNode && nodeTree.isOverlay(liveNode) && liveNode->isVisible() && liveNode->isEnabled();
    }

    bool ModalSystem::eraseInvalidModalSession(NodeTree &nodeTree, InputSystem &input, size_t index)
    {
        if (index >= modals_.size())
            return false;

        const ModalSession session = modals_[index];
        Node *modalNode = nodeTree.findNode(session.modalId);
        if (modalNode && isLiveVisibleEnabledModal(nodeTree, *modalNode))
            return false;

        modals_.erase(modals_.begin() + static_cast<std::ptrdiff_t>(index), modals_.end());

        input.cancelPointerInteraction(nodeTree);
        input.clearFocus(nodeTree);
        restoreFocusAfterClose(nodeTree, input, session);
        startBackdropAnimation(nodeTree);
        return true;
    }

    void ModalSystem::focusOrClear(NodeTree &nodeTree, InputSystem &input, Node *focus) const
    {
        if (focus)
            input.focus(nodeTree, *focus);
        else
            input.clearFocus(nodeTree);
    }

    void ModalSystem::ensureBackdrop(NodeTree &nodeTree)
    {
        if (backdropNode_ && backdropId_ && nodeTree.findNode(*backdropId_) == backdropNode_)
            return;

        backdropNode_ = nullptr;
        backdropId_.reset();

        auto backdrop = std::make_unique<BackdropNode>();
        backdrop->setVisible(true);
        backdrop->setEnabled(true);
        backdrop->setFocusable(false);
        backdrop->setCapturable(false);
        backdrop->setPosition({0.0f, 0.0f});
        backdrop->setPositionMode(PositionMode::Absolute);
        backdrop->setSize(LayoutSizeValue::fixed(viewportSize_.width, viewportSize_.height));
        backdrop->setBackdrop(backdropColor_, backdropOpacity_);

        Node *raw = nodeTree.attachOverlay(0, std::move(backdrop));
        if (!raw)
            return;

        backdropNode_ = dynamic_cast<BackdropNode *>(raw);
        if (!backdropNode_)
            return;

        backdropId_ = backdropNode_->getId();
    }

    void ModalSystem::removeBackdrop(NodeTree &nodeTree) noexcept
    {
        if (!backdropId_)
        {
            backdropNode_ = nullptr;
            return;
        }

        if (Node *liveBackdrop = nodeTree.findNode(*backdropId_))
        {
            liveBackdrop->cancelAnimation(&backdropOpacity_);
            nodeTree.removeOverlay(liveBackdrop);
        }

        backdropNode_ = nullptr;
        backdropId_.reset();
    }

    void ModalSystem::updateBackdropState() noexcept
    {
        bool shouldShow = false;
        for (const ModalSession &session : modals_)
        {
            if (session.options.showBackdrop)
            {
                shouldShow = true;
                break;
            }
        }

        backdropTargetOpacity_ = shouldShow ? 1.0f : 0.0f;
    }

    void ModalSystem::startBackdropAnimation(NodeTree &nodeTree) noexcept
    {
        updateBackdropState();

        if (backdropTargetOpacity_ > 0.0f)
            ensureBackdrop(nodeTree);

        if (!backdropNode_)
            return;

        backdropNode_->setBackdrop(backdropColor_, backdropOpacity_);
        backdropNode_->animateFloat(
            &backdropOpacity_,
            backdropOpacity_,
            backdropTargetOpacity_,
            backdropFadeDuration_,
            AnimationEasing::EaseOut,
            [this](float value)
            {
                backdropOpacity_ = std::clamp(value, 0.0f, 1.0f);
                if (backdropNode_)
                    backdropNode_->setBackdrop(backdropColor_, backdropOpacity_);
            });
    }
}
