#include "scroll_system.hpp"

#include <algorithm>
#include <cmath>

#include "node_tree.hpp"
#include "ui_framework/panel_node.hpp"

namespace
{
    float finiteOrZero(float value) noexcept
    {
        return std::isfinite(value) ? value : 0.0f;
    }
}

namespace ui
{
    ScrollOffset ScrollState::maxOffset(const LayoutSize &viewport) const noexcept
    {
        return {
            std::max(0.0f, content.width - viewport.width),
            std::max(0.0f, content.height - viewport.height)};
    }

    void ScrollState::clampOffset(const LayoutSize &viewport) noexcept
    {
        const ScrollOffset maximum = maxOffset(viewport);
        offset.x = std::clamp(finiteOrZero(offset.x), 0.0f, maximum.x);
        offset.y = std::clamp(finiteOrZero(offset.y), 0.0f, maximum.y);
    }

    bool ScrollSystem::registerScrollNode(NodeTree &nodeTree, Node &node)
    {
        if (!nodeTree.isNodeLive(node.getId()) || nodeTree.findNode(node.getId()) != &node)
            return false;
        if (!dynamic_cast<PanelNode *>(&node))
            return false;
        if (states_.contains(node.getId()))
            return false;

        states_.emplace(node.getId(), ScrollState{});
        return true;
    }

    bool ScrollSystem::unregisterScrollNode(NodeTree &nodeTree, Node::Id nodeId)
    {
        if (!states_.contains(nodeId))
            return false;

        (void)nodeTree;
        states_.erase(nodeId);
        return true;
    }

    bool ScrollSystem::isRegistered(Node::Id nodeId) const noexcept
    {
        return states_.contains(nodeId);
    }

    bool ScrollSystem::setOffset(NodeTree &nodeTree, Node::Id nodeId, const ScrollOffset &offset)
    {
        auto it = states_.find(nodeId);
        Node *node = nodeTree.findNode(nodeId);
        if (it == states_.end() || !node)
            return false;

        it->second.offset = offset;
        it->second.clampOffset(getViewport(*node));
        return true;
    }

    bool ScrollSystem::scrollBy(NodeTree &nodeTree, Node::Id nodeId, const ScrollOffset &delta)
    {
        auto it = states_.find(nodeId);
        Node *node = nodeTree.findNode(nodeId);
        if (it == states_.end() || !node)
            return false;

        it->second.offset.x += finiteOrZero(delta.x);
        it->second.offset.y += finiteOrZero(delta.y);
        it->second.clampOffset(getViewport(*node));
        return true;
    }

    std::optional<ScrollState> ScrollSystem::getState(Node::Id nodeId) const
    {
        const auto it = states_.find(nodeId);
        if (it == states_.end())
            return std::nullopt;
        return it->second;
    }

    ScrollOffset ScrollSystem::getOffset(Node::Id nodeId) const noexcept
    {
        const auto it = states_.find(nodeId);
        return it == states_.end() ? ScrollOffset{} : it->second.offset;
    }

    ScrollOffset ScrollSystem::getMaxOffset(const Node &node) const noexcept
    {
        const auto it = states_.find(node.getId());
        if (it == states_.end())
            return {};
        return it->second.maxOffset(getViewport(node));
    }

    ScrollOffset ScrollSystem::getAccumulatedOffset(const Node &node) const noexcept
    {
        ScrollOffset result{};
        const Node *current = node.getParent();
        while (current)
        {
            if (const auto it = states_.find(current->getId()); it != states_.end())
            {
                result.x += it->second.offset.x;
                result.y += it->second.offset.y;
            }
            current = current->getParent();
        }
        return result;
    }

    Node *ScrollSystem::findNearestScrollableAncestor(NodeTree &, Node *target) const noexcept
    {
        for (Node *current = target; current; current = current->getParent())
            if (isRegistered(current->getId()))
                return current;
        return nullptr;
    }

    LayoutSize ScrollSystem::getViewport(const Node &node) const noexcept
    {
        const Padding padding = node.getPadding();
        const Border border = node.getBorder();
        const LayoutSize size = node.getActualSize();
        return {
            std::max(0.0f, size.width - padding.left - padding.right - border.left - border.right),
            std::max(0.0f, size.height - padding.top - padding.bottom - border.top - border.bottom)};
    }

    void ScrollSystem::calculateContentExtentRecursive(
        const Node &root,
        const Node &current,
        float originX,
        float originY,
        float &maxRight,
        float &maxBottom) const noexcept
    {
        if (&current != &root)
        {
            const LayoutPosition position = current.getActualPosition();
            const LayoutSize size = current.getActualSize();
            maxRight = std::max(maxRight, finiteOrZero(position.x) - originX + std::max(0.0f, finiteOrZero(size.width)));
            maxBottom = std::max(maxBottom, finiteOrZero(position.y) - originY + std::max(0.0f, finiteOrZero(size.height)));
        }

        if (&current != &root && isRegistered(current.getId()))
            return;

        const auto *panel = dynamic_cast<const PanelNode *>(&current);
        if (!panel)
            return;

        for (size_t index = 0; index < panel->getChildCount(); ++index)
        {
            const Node *child = panel->getChild(index);
            if (child && child->isVisible())
                calculateContentExtentRecursive(root, *child, originX, originY, maxRight, maxBottom);
        }
    }

    LayoutSize ScrollSystem::calculateContentExtent(const Node &node) const noexcept
    {
        Node::ScopedCoordinateTransform rawLayoutTransform([](const Node &, const LayoutPosition &position)
        {
            return position;
        });

        const LayoutPosition position = node.getActualPosition();
        const Padding padding = node.getPadding();
        const Border border = node.getBorder();
        const float originX = finiteOrZero(position.x) + padding.left + border.left;
        const float originY = finiteOrZero(position.y) + padding.top + border.top;

        float maxRight = 0.0f;
        float maxBottom = 0.0f;
        calculateContentExtentRecursive(node, node, originX, originY, maxRight, maxBottom);
        return {maxRight, maxBottom};
    }

    ScrollOffset ScrollSystem::consumeScroll(Node &node, ScrollOffset delta) noexcept
    {
        auto it = states_.find(node.getId());
        if (it == states_.end())
            return delta;

        const ScrollOffset maximum = it->second.maxOffset(getViewport(node));
        const ScrollOffset before = it->second.offset;

        it->second.offset.x = std::clamp(before.x + finiteOrZero(delta.x), 0.0f, maximum.x);
        it->second.offset.y = std::clamp(before.y + finiteOrZero(delta.y), 0.0f, maximum.y);

        return {
            delta.x - (it->second.offset.x - before.x),
            delta.y - (it->second.offset.y - before.y)};
    }

    bool ScrollSystem::handleWheel(
        NodeTree &nodeTree,
        float x,
        float y,
        float deltaX,
        float deltaY,
        const Node *modalRoot)
    {
        Node *target = nodeTree.hitTest(x, y, modalRoot);
        ScrollOffset remaining{finiteOrZero(deltaX), finiteOrZero(deltaY)};
        bool consumed = false;

        Node *scrollNode = findNearestScrollableAncestor(nodeTree, target);
        while (scrollNode)
        {
            const ScrollOffset before = remaining;
            remaining = consumeScroll(*scrollNode, remaining);
            consumed = consumed || before.x != remaining.x || before.y != remaining.y;

            if (remaining.x == 0.0f && remaining.y == 0.0f)
                break;

            Node *parent = scrollNode->getParent();
            scrollNode = findNearestScrollableAncestor(nodeTree, parent);
        }

        return consumed;
    }

    void ScrollSystem::sync(NodeTree &nodeTree)
    {
        for (auto it = states_.begin(); it != states_.end(); )
        {
            Node *node = nodeTree.findNode(it->first);
            if (!node || !dynamic_cast<PanelNode *>(node))
            {
                it = states_.erase(it);
                continue;
            }

            it->second.content = calculateContentExtent(*node);
            it->second.clampOffset(getViewport(*node));
            ++it;
        }
    }

    void ScrollSystem::clear(NodeTree &) noexcept
    {
        states_.clear();
    }
}
