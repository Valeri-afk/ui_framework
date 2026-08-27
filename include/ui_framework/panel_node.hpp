#pragma once

#include "node.hpp"
#include "ui_framework/types.hpp"

namespace ui
{
    class Node;

    class PanelNode : public Node
    {
    public:
        PanelNode();
        ~PanelNode() override;

        Node *addChild(std::unique_ptr<Node> child, size_t index);
        void removeChild(Node &child);

        size_t getChildCount() const noexcept;
        bool hasChildren() const noexcept;

        Node *getChild(size_t index) noexcept;
        const Node *getChild(size_t index) const noexcept;

        void forEachChild(const std::function<bool(Node &)> &cb);
        void forEachChildReverse(const std::function<bool(Node &)> &cb);

    protected:
        bool canAttach(const Node &child) const noexcept;
        bool isAncestorOf(const Node *node) const noexcept;

    private:
        Node *getVisibleChild(size_t visibleIndex) const noexcept override;
        size_t getVisibleChildCount() const noexcept;
        size_t getVisibleChildIndexAt(size_t childIndex) const noexcept;

        Node *attachLocal(std::unique_ptr<Node> child, size_t index);
        std::unique_ptr<Node> detachLocal(Node &child);

        void forEachChildImpl(
            const std::function<bool(Node &)> &cb,
            bool reverse);

        std::vector<std::unique_ptr<Node>> children_;

        friend class NodeTree;
    };
}
