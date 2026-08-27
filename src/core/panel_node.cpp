#include "ui_framework/panel_node.hpp"

#include "node_tree.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>

namespace
{
    float finiteOrZero(float value) noexcept { return std::isfinite(value) ? value : 0.0f; }
    int clampToInt(float value) noexcept
    {
        if (!std::isfinite(value)) return 0;
        if (value <= static_cast<float>(std::numeric_limits<int>::min())) return std::numeric_limits<int>::min();
        if (value >= static_cast<float>(std::numeric_limits<int>::max())) return std::numeric_limits<int>::max();
        return static_cast<int>(value);
    }
    std::optional<std::size_t> findChildIndexById(const std::vector<std::unique_ptr<ui::Node>> &children, ui::Node::Id id)
    {
        for (std::size_t i = 0; i < children.size(); ++i)
            if (children[i] && children[i]->getId() == id) return i;
        return std::nullopt;
    }
}

namespace ui
{
    PanelNode::PanelNode() = default;
    PanelNode::~PanelNode() = default;

    Node *PanelNode::addChild(std::unique_ptr<Node> child, size_t index)
    {
        if (!child) return nullptr;
        if (owner_) return owner_->attachChild(*this, std::move(child), index);
        return attachLocal(std::move(child), index);
    }

    void PanelNode::removeChild(Node &child)
    {
        if (owner_)
        {
            owner_->removeChild(*this, child);
            return;
        }
        detachLocal(child).reset();
    }

    size_t PanelNode::getChildCount() const noexcept { return children_.size(); }
    bool PanelNode::hasChildren() const noexcept { return !children_.empty(); }
    Node *PanelNode::getChild(size_t index) noexcept { return index < children_.size() ? children_[index].get() : nullptr; }
    const Node *PanelNode::getChild(size_t index) const noexcept { return index < children_.size() ? children_[index].get() : nullptr; }

    void PanelNode::forEachChild(const std::function<bool(Node &)> &cb)
    {
        if (!cb) return;
        forEachChildImpl(cb, false);
    }

    void PanelNode::forEachChildReverse(const std::function<bool(Node &)> &cb)
    {
        if (!cb) return;
        forEachChildImpl(cb, true);
    }

    void PanelNode::forEachChildImpl(const std::function<bool(Node &)> &cb, bool reverse)
    {
        if (!cb) return;
        auto iterate = [&]()
        {
            std::vector<Node::Id> snapshot;
            snapshot.reserve(children_.size());
            if (reverse)
            {
                for (auto it = children_.rbegin(); it != children_.rend(); ++it)
                    if (*it) snapshot.push_back((*it)->getId());
            }
            else
            {
                for (const auto &child : children_)
                    if (child) snapshot.push_back(child->getId());
            }
            for (Node::Id id : snapshot)
            {
                const auto index = findChildIndexById(children_, id);
                if (!index) continue;
                Node *child = children_[*index].get();
                if (!child || child->getId() != id) continue;
                if (cb(*child)) return;
            }
        };
        NodeTree *tree = owner_;
        if (!tree) { iterate(); return; }
        { NodeTree::ScopedMutationGuard guard(*tree); iterate(); }
        tree->flushMutationQueue();
    }

    Node *PanelNode::getVisibleChild(size_t visibleIndex) const noexcept
    {
        size_t current = 0;
        for (const auto &child : children_)
        {
            if (!child || !child->isVisible()) continue;
            if (current == visibleIndex) return child.get();
            ++current;
        }
        return nullptr;
    }

    size_t PanelNode::getVisibleChildCount() const noexcept
    {
        size_t result = 0;
        for (const auto &child : children_)
            if (child && child->isVisible()) ++result;
        return result;
    }

    size_t PanelNode::getVisibleChildIndexAt(size_t childIndex) const noexcept
    {
        if (childIndex >= children_.size()) return getVisibleChildCount();
        size_t result = 0;
        for (size_t i = 0; i < childIndex; ++i)
            if (children_[i] && children_[i]->isVisible()) ++result;
        return result;
    }

    Node *PanelNode::attachLocal(std::unique_ptr<Node> child, size_t index)
    {
        if (!child || !canAttach(*child)) return nullptr;
        if (index > children_.size()) index = children_.size();
        children_.insert(children_.begin() + static_cast<std::ptrdiff_t>(index), std::move(child));
        children_[index]->parent_ = this;
        return children_[index].get();
    }

    std::unique_ptr<Node> PanelNode::detachLocal(Node &child)
    {
        for (auto it = children_.begin(); it != children_.end(); ++it)
        {
            if (it->get() == &child)
            {
                auto result = std::move(*it);
                children_.erase(it);
                result->parent_ = nullptr;
                return result;
            }
        }
        return nullptr;
    }

    bool PanelNode::canAttach(const Node &child) const noexcept
    {
        if (isAncestorOf(&child))
        {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "PanelNode hierarchy cycle detected.");
            return false;
        }
        if (child.parent_)
        {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "PanelNode child already has parent.");
            return false;
        }
        if (child.owner_)
        {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "PanelNode child already belongs to a tree.");
            return false;
        }
        return true;
    }

    bool PanelNode::isAncestorOf(const Node *node) const noexcept
    {
        const Node *current = node;
        while (current)
        {
            if (current == this) return true;
            current = current->parent_;
        }
        return false;
    }
}
