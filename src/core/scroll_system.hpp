#pragma once

#include <optional>
#include <unordered_map>

#include "ui_framework/node.hpp"
#include "ui_framework/types.hpp"

namespace ui
{
    class NodeTree;

    struct ScrollState
    {
        LayoutSize content{};
        ScrollOffset offset{};

        ScrollOffset maxOffset(const LayoutSize &viewport) const noexcept;
        void clampOffset(const LayoutSize &viewport) noexcept;
    };

    class ScrollSystem
    {
    public:
        ScrollSystem() = default;
        ScrollSystem(const ScrollSystem &) = delete;
        ScrollSystem &operator=(const ScrollSystem &) = delete;

        bool registerScrollNode(NodeTree &nodeTree, Node &node);
        bool unregisterScrollNode(NodeTree &nodeTree, Node::Id nodeId);
        bool isRegistered(Node::Id nodeId) const noexcept;

        bool setOffset(NodeTree &nodeTree, Node::Id nodeId, const ScrollOffset &offset);
        bool scrollBy(NodeTree &nodeTree, Node::Id nodeId, const ScrollOffset &delta);
        std::optional<ScrollState> getState(Node::Id nodeId) const;
        ScrollOffset getOffset(Node::Id nodeId) const noexcept;
        ScrollOffset getMaxOffset(const Node &node) const noexcept;
        ScrollOffset getAccumulatedOffset(const Node &node) const noexcept;

        Node *findNearestScrollableAncestor(NodeTree &nodeTree, Node *target) const noexcept;
        bool handleWheel(NodeTree &nodeTree, float x, float y, float deltaX, float deltaY, const Node *modalRoot = nullptr);

        void sync(NodeTree &nodeTree);
        void clear(NodeTree &nodeTree) noexcept;

    private:
        LayoutSize getViewport(const Node &node) const noexcept;
        LayoutSize calculateContentExtent(const Node &node) const noexcept;
        void calculateContentExtentRecursive(
            const Node &root,
            const Node &current,
            float originX,
            float originY,
            float &maxRight,
            float &maxBottom) const noexcept;
        ScrollOffset consumeScroll(Node &node, ScrollOffset delta) noexcept;

        std::unordered_map<Node::Id, ScrollState> states_;
    };
}