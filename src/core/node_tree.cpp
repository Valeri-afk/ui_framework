#include "node_tree.hpp"
#include "rendering_state.hpp"
#include <unordered_set>

#include <algorithm>
#include <optional>
#include <utility>

namespace
{
    static std::optional<size_t> findChildIndexById(
        const std::vector<std::unique_ptr<ui::Node>> &children,
        ui::Node::Id id)
    {
        for (size_t i = 0; i < children.size(); ++i)
        {
            if (children[i] && children[i]->getId() == id)
                return i;
        }

        return std::nullopt;
    }

    thread_local std::unordered_set<ui::Node::Id> unmountingNodeIds;
}

namespace ui
{
    NodeTree::ScopedMutationGuard::ScopedMutationGuard(
        NodeTree &tree) noexcept
        : tree_(&tree)
    {
        tree_->enterMutationScope();
    }

    NodeTree::ScopedMutationGuard::~ScopedMutationGuard()
    {
        if (tree_)
        {
            tree_->leaveMutationScope();
        }
    }

    void NodeTree::enterMutationScope() noexcept
    {
        ++mutationDepth_;
    }

    void NodeTree::leaveMutationScope() noexcept
    {
        SDL_assert(mutationDepth_ > 0);

        if (mutationDepth_ > 0)
        {
            --mutationDepth_;
        }
    }

    bool NodeTree::isMutationScopeActive() const noexcept
    {
        return mutationDepth_ > 0;
    }

    void NodeTree::flushMutationQueue()
    {
        if (isMutationScopeActive())
            return;

        ScopedMutationGuard guard(*this);

        drainMutationQueue();
    }

    void NodeTree::drainMutationQueue()
    {
        while (!mutationQueue_.empty())
        {
            std::vector<std::unique_ptr<Mutation>> queue;
            queue.swap(mutationQueue_);

            for (auto &mutation : queue)
            {
                if (mutation)
                {
                    (*mutation)();
                }
            }
        }
    }

    void NodeTree::assertSubtreeOwner(const Node &node, NodeTree *owner) const
    {
#ifndef NDEBUG
        SDL_assert(node.owner_ == owner);

        if (const PanelNode *panel = dynamic_cast<const PanelNode *>(&node))
        {
            for (const auto &child : panel->children_)
            {
                if (child)
                {
                    assertSubtreeOwner(*child, owner);
                }
            }
        }
#else
        (void)node;
        (void)owner;
#endif
    }

    void NodeTree::assertSubtreeLive(const Node &node) const
    {
#ifndef NDEBUG
        SDL_assert(findNode(node.getId()) == &node);

        if (const PanelNode *panel = dynamic_cast<const PanelNode *>(&node))
        {
            for (const auto &child : panel->children_)
            {
                if (child)
                {
                    assertSubtreeLive(*child);
                }
            }
        }
#else
        (void)node;
#endif
    }

    void NodeTree::assertLiveSubtree(NodeId id, NodeTree *owner) const
    {
#ifndef NDEBUG
        if (const Node *live = findNode(id))
        {
            assertSubtreeOwner(*live, owner);
            assertSubtreeLive(*live);
        }
#else
        (void)id;
        (void)owner;
#endif
    }

    bool NodeTree::containsNodeInContainer(
        const std::vector<std::unique_ptr<Node>> &container,
        NodeId id) const noexcept
    {
        return std::any_of(
            container.begin(),
            container.end(),
            [id](const std::unique_ptr<Node> &node)
            {
                return node && node->getId() == id;
            });
    }

    PanelNode *NodeTree::resolveLivePanelNode(NodeId id)
    {
        Node *liveNode = findNode(id);

        if (!liveNode)
            return nullptr;

        PanelNode *panel = dynamic_cast<PanelNode *>(liveNode);

        if (!panel)
        {
            SDL_LogError(
                SDL_LOG_CATEGORY_APPLICATION,
                "NodeTree: parent is not a PanelNode.");

            SDL_assert(false);
            return nullptr;
        }

        return panel;
    }

    PanelNode *NodeTree::resolveLivePanelParent(Node &parent)
    {
        return resolveLivePanelNode(parent.getId());
    }

    void NodeTree::registerNode(Node &node)
    {
#ifndef NDEBUG
        auto it = liveNodes_.find(node.getId());
        SDL_assert(it == liveNodes_.end() || it->second == &node);
#endif

        liveNodes_[node.getId()] = &node;
    }

    void NodeTree::unregisterNode(Node &node)
    {
#ifndef NDEBUG
        auto it = liveNodes_.find(node.getId());
        SDL_assert(it != liveNodes_.end());

        if (it != liveNodes_.end())
        {
            SDL_assert(it->second == &node);
        }
#endif

        liveNodes_.erase(node.getId());
    }

    void NodeTree::registerSubtree(Node &root)
    {
        traversePreOrder(
            root,
            [this](Node &node)
            {
                registerNode(node);
                return TraversalResult::Continue;
            });
    }

    void NodeTree::unregisterSubtree(Node &root)
    {
        traversePostOrder(
            root,
            [this](Node &node)
            {
                unregisterNode(node);
                return TraversalResult::Continue;
            });
    }

    void NodeTree::setSubtreeOwner(
        Node &root,
        NodeTree *owner)
    {
        traversePreOrder(
            root,
            [owner](Node &node)
            {
                node.owner_ = owner;
                return TraversalResult::Continue;
            });
    }

    void NodeTree::attachOwnedSubtree(Node &root, NodeId rootId)
    {
        setSubtreeOwner(root, this);
        registerSubtree(root);

#ifndef NDEBUG
        assertSubtreeOwner(root, this);
        assertSubtreeLive(root);
#endif

        if (!findNode(rootId))
            return;

        mountSubtree(root);

        assertLiveSubtree(rootId, this);
    }

    void NodeTree::detachOwnedSubtree(
        Node &root,
        NodeId rootId)
    {
#ifndef NDEBUG
        SDL_assert(root.owner_ == this);
        assertSubtreeLive(root);
#endif

        unmountSubtree(root);

        if (findNode(rootId))
        {
            unregisterSubtree(root);
        }

        setSubtreeOwner(root, nullptr);

#ifndef NDEBUG
        SDL_assert(root.owner_ == nullptr);
#endif
    }

    Node *NodeTree::findNode(NodeId id)
    {
        auto it = liveNodes_.find(id);
        return it == liveNodes_.end() ? nullptr : it->second;
    }

    const Node *NodeTree::findNode(NodeId id) const
    {
        auto it = liveNodes_.find(id);
        return it == liveNodes_.end() ? nullptr : it->second;
    }

    bool NodeTree::isNodeLive(NodeId id) const
    {
        return liveNodes_.find(id) != liveNodes_.end();
    }

    bool NodeTree::isRoot(const Node *node) const noexcept
    {
        if (!node)
            return false;

        return containsNodeInContainer(roots_, node->getId());
    }

    bool NodeTree::isOverlay(const Node *node) const noexcept
    {
        if (!node)
            return false;

        return containsNodeInContainer(overlays_, node->getId());
    }

    void NodeTree::requestFullLayout()
    {
        for (const auto &root : roots_)
        {
            insertLayoutQueue(root.get());
        }

        for (const auto &overlay : overlays_)
        {
            insertLayoutQueue(overlay.get());
        }
    }

    Node *NodeTree::attachRoot(
        size_t index,
        std::unique_ptr<Node> node)
    {
        return attachToContainer(index, std::move(node), roots_);
    }

    Node *NodeTree::attachOverlay(
        size_t index,
        std::unique_ptr<Node> node)
    {
        return attachToContainer(index, std::move(node), overlays_);
    }

    void NodeTree::removeRoot(Node *node)
    {
        if (!node)
            return;

        removeFromContainer(node->getId(), roots_);
    }

    void NodeTree::removeOverlay(Node *node)
    {
        if (!node)
            return;

        removeFromContainer(node->getId(), overlays_);
    }

    Node *NodeTree::attachChild(
        PanelNode &parent,
        std::unique_ptr<Node> child,
        size_t index)
    {
        if (isMutationScopeActive())
        {
            const NodeId parentId = parent.getId();

            if (!findNode(parentId))
                return nullptr;

            enqueueMutation(
                [this, parentId, child = std::move(child), index]() mutable
                {
                    if (PanelNode *panelParent = resolveLivePanelNode(parentId))
                    {
                        attachChildInternal(
                            *panelParent,
                            std::move(child),
                            index);
                    }
                });

            return nullptr;
        }

        PanelNode *panelParent = resolveLivePanelParent(parent);

        if (!panelParent)
            return nullptr;

        return attachChildInternal(
            *panelParent,
            std::move(child),
            index);
    }

    void NodeTree::removeChild(
        PanelNode &parent,
        Node &child)
    {
        if (isMutationScopeActive())
        {
            const NodeId parentId = parent.getId();
            const NodeId childId = child.getId();

            if (!findNode(parentId) || !findNode(childId))
                return;

            enqueueMutation(
                [this, parentId, childId]()
                {
                    if (unmountingNodeIds.contains(childId))
                        return;

                    if (PanelNode *panelParent = resolveLivePanelNode(parentId))
                    {
                        if (Node *liveChild = findNode(childId))
                        {
                            if (liveChild->getParent() == panelParent)
                            {
                                removeChildInternal(*panelParent, *liveChild);
                            }
                        }
                    }
                });

            return;
        }

        PanelNode *panelParent = resolveLivePanelParent(parent);

        if (!panelParent)
            return;

        removeChildInternal(*panelParent, child);
    }

    Node *NodeTree::attachToContainer(
        size_t index,
        std::unique_ptr<Node> node,
        std::vector<std::unique_ptr<Node>> &container)
    {
        if (isMutationScopeActive())
        {
            enqueueMutation(
                [this, index, node = std::move(node), &container]() mutable
                {
                    attachInternal(index, std::move(node), container);
                });

            return nullptr;
        }

        return attachInternal(index, std::move(node), container);
    }

    void NodeTree::removeFromContainer(
        NodeId id,
        std::vector<std::unique_ptr<Node>> &container)
    {
        if (isMutationScopeActive())
        {
            enqueueMutation(
                [this, id, &container]()
                {
                    removeInternal(id, container);
                });

            return;
        }

        removeInternal(id, container);
    }

    void NodeTree::flushMutationQueueAndInsertLayout(NodeId id)
    {
        flushMutationQueue();

        if (Node *liveNode = findNode(id))
        {
            insertLayoutQueue(liveNode);
        }
    }

    Node *NodeTree::attachInternal(
        size_t index,
        std::unique_ptr<Node> node,
        std::vector<std::unique_ptr<Node>> &container)
    {
        if (!node)
            return nullptr;

#ifndef NDEBUG
        SDL_assert(node->getParent() == nullptr);
        SDL_assert(node->owner_ == nullptr);
#endif

        if (node->getParent() || node->owner_)
        {
            SDL_LogError(
                SDL_LOG_CATEGORY_APPLICATION,
                "NodeTree: node already belongs to a tree.");

            SDL_assert(false);
            return nullptr;
        }

        if (index > container.size())
            index = container.size();

        Node *raw = node.get();
        const NodeId nodeId = raw->getId();

        container.insert(
            container.begin() + static_cast<std::ptrdiff_t>(index),
            std::move(node));

        attachOwnedSubtree(*raw, nodeId);

        flushMutationQueueAndInsertLayout(nodeId);

        assertLiveSubtree(nodeId, this);

        return findNode(nodeId);
    }

    void NodeTree::removeInternal(
        NodeId id,
        std::vector<std::unique_ptr<Node>> &container)
    {
        auto it = std::find_if(
            container.begin(),
            container.end(),
            [id](const std::unique_ptr<Node> &node)
            {
                return node && node->getId() == id;
            });

        if (it == container.end())
            return;

        auto removed = std::move(*it);

        container.erase(it);

        detachOwnedSubtree(*removed, id);

        // `removed` remains the temporary owner while the subtree is
        // unmounted and unregistered. It is destroyed here, after all
        // runtime/lifecycle bookkeeping has completed.
    }

    Node *NodeTree::attachChildInternal(
        PanelNode &parent,
        std::unique_ptr<Node> child,
        size_t index)
    {
        if (!child)
            return nullptr;

        const NodeId parentId = parent.getId();

        if (parent.owner_ != this)
        {
            SDL_LogError(
                SDL_LOG_CATEGORY_APPLICATION,
                "NodeTree: parent does not belong to this tree.");

            SDL_assert(false);
            return nullptr;
        }

        if (child->getParent() || child->owner_)
        {
            SDL_LogError(
                SDL_LOG_CATEGORY_APPLICATION,
                "NodeTree: child already belongs to a tree.");

            SDL_assert(false);
            return nullptr;
        }

        Node *raw = child.get();
        const NodeId childId = raw->getId();

        Node *attached =
            parent.attachLocal(std::move(child), index);

        if (!attached)
            return nullptr;

        attachOwnedSubtree(*raw, childId);

        flushMutationQueue();

        if (Node *liveParent = findNode(parentId))
        {
            insertLayoutQueue(liveParent);
        }

        if (Node *liveChild = findNode(childId))
        {
            return liveChild;
        }

        return nullptr;
    }

    void NodeTree::removeChildInternal(
        PanelNode &parent,
        Node &child)
    {
        const NodeId parentId = parent.getId();
        const NodeId childId = child.getId();

        if (parent.owner_ != this)
        {
            SDL_LogError(
                SDL_LOG_CATEGORY_APPLICATION,
                "NodeTree: parent does not belong to this tree.");

            SDL_assert(false);
            return;
        }

        if (child.owner_ != this)
        {
            SDL_LogError(
                SDL_LOG_CATEGORY_APPLICATION,
                "NodeTree: child does not belong to this tree.");

            SDL_assert(false);
            return;
        }

        if (child.getParent() != &parent)
        {
            SDL_LogError(
                SDL_LOG_CATEGORY_APPLICATION,
                "NodeTree: child is not a child of the given parent.");

            SDL_assert(false);
            return;
        }

        auto removed =
            parent.detachLocal(child);

        if (!removed)
            return;

#ifndef NDEBUG
        SDL_assert(removed->owner_ == this);
        SDL_assert(removed->getParent() == nullptr);
        assertSubtreeLive(*removed);
#endif

        detachOwnedSubtree(*removed, childId);

        if (Node *liveParent = findNode(parentId))
        {
            insertLayoutQueue(liveParent);
        }

        // `removed` is the temporary framework owner and is destroyed
        // when this function returns.
    }

    void NodeTree::advanceTime(float dt) noexcept
    {
        animationSystem_.advance(dt);
    }

    NodeTree::TraversalResult NodeTree::traversePreOrder(
        Node &node,
        const TraversalCallback &callback,
        bool reverse)
    {
        if (!callback)
            return TraversalResult::Continue;

        const TraversalResult result =
            callback(node);

        if (result == TraversalResult::Stop)
            return TraversalResult::Stop;

        if (result == TraversalResult::SkipChildren)
            return TraversalResult::Continue;

        auto *panel = dynamic_cast<PanelNode *>(&node);

        if (!panel)
            return TraversalResult::Continue;

        std::vector<NodeId> snapshot;
        snapshot.reserve(panel->children_.size());

        if (reverse)
        {
            for (auto it = panel->children_.rbegin();
                 it != panel->children_.rend();
                 ++it)
            {
                if (*it)
                    snapshot.push_back((*it)->getId());
            }
        }
        else
        {
            for (const auto &child : panel->children_)
            {
                if (child)
                    snapshot.push_back(child->getId());
            }
        }

        for (NodeId childId : snapshot)
        {
            const auto index =
                findChildIndexById(
                    panel->children_,
                    childId);

            if (!index)
                continue;

            Node *child =
                panel->children_[*index].get();

            if (!child ||
                child->getId() != childId ||
                child->parent_ != panel)
            {
                continue;
            }

            const TraversalResult childResult =
                traversePreOrder(
                    *child,
                    callback,
                    reverse);

            if (childResult == TraversalResult::Stop)
                return TraversalResult::Stop;
        }

        return TraversalResult::Continue;
    }

    NodeTree::TraversalResult NodeTree::traversePostOrder(
        Node &node,
        const TraversalCallback &callback,
        bool reverse)
    {
        if (!callback)
            return TraversalResult::Continue;

        auto *panel = dynamic_cast<PanelNode *>(&node);

        if (panel)
        {
            std::vector<NodeId> snapshot;
            snapshot.reserve(panel->children_.size());

            if (reverse)
            {
                for (const auto &child : panel->children_)
                {
                    if (child)
                        snapshot.push_back(child->getId());
                }
            }
            else
            {
                for (auto it = panel->children_.rbegin();
                     it != panel->children_.rend();
                     ++it)
                {
                    if (*it)
                        snapshot.push_back((*it)->getId());
                }
            }

            for (NodeId childId : snapshot)
            {
                const auto index =
                    findChildIndexById(
                        panel->children_,
                        childId);

                if (!index)
                    continue;

                Node *child =
                    panel->children_[*index].get();

                if (!child ||
                    child->getId() != childId ||
                    child->parent_ != panel)
                {
                    continue;
                }

                const TraversalResult childResult =
                    traversePostOrder(
                        *child,
                        callback,
                        reverse);

                if (childResult == TraversalResult::Stop)
                    return TraversalResult::Stop;
            }
        }

        return callback(node) == TraversalResult::Stop
                   ? TraversalResult::Stop
                   : TraversalResult::Continue;
    }

    void NodeTree::mountSubtree(Node &root)
    {
        {
            ScopedMutationGuard guard(*this);

            traversePreOrder(
                root,
                [](Node &node)
                {
                    node.onMount();
                    return TraversalResult::Continue;
                });
        }

        flushMutationQueue();
    }

    void NodeTree::unmountSubtree(Node &root)
    {
        std::vector<NodeId> unmountingIds;

        traversePreOrder(
            root,
            [&unmountingIds](Node &node)
            {
                unmountingIds.push_back(node.getId());
                return TraversalResult::Continue;
            });

        for (NodeId id : unmountingIds)
        {
            unmountingNodeIds.insert(id);
        }

        {
            ScopedMutationGuard guard(*this);

            traversePostOrder(
                root,
                [](Node &node)
                {
                    node.onUnmount();
                    return TraversalResult::Continue;
                });
        }

        flushMutationQueue();

        for (NodeId id : unmountingIds)
        {
            unmountingNodeIds.erase(id);
        }
    }

    void NodeTree::insertLayoutQueue(Node *node)
    {
        if (!node || findNode(node->getId()) != node)
            return;

        Node *root = node;

        while (root->getParent())
        {
            root = root->getParent();
        }

        insertLayoutQueueById(root->getId());
    }

    void NodeTree::insertLayoutQueueById(NodeId id)
    {
        Node *node = findNode(id);

        if (!node)
            return;

        Node *layoutRoot = node;

        while (layoutRoot->getParent())
        {
            layoutRoot = layoutRoot->getParent();
        }

        if (!isRoot(layoutRoot) && !isOverlay(layoutRoot))
            return;

        if (layoutQueueSet_.insert(layoutRoot->getId()).second)
        {
            layoutQueue_.push_back(layoutRoot->getId());
        }
    }

    void NodeTree::forEachLayoutQueue(
        const std::function<void(Node &)> &cb)
    {
        std::deque<NodeId> queue;
        queue.swap(layoutQueue_);
        layoutQueueSet_.clear();

        for (NodeId id : queue)
        {
            if (Node *node = findNode(id))
            {
                cb(*node);
            }
        }
    }

    void NodeTree::drawSubtree(
        Node &node,
        SDL_Renderer *renderer)
    {
        if (!renderer || !node.isVisible())
            return;

        RendererStateScope state(renderer);

        if (node.getClipToBounds())
        {
            SDL_Rect nodeRect =
                toSDLRect(node);

            SDL_Rect previousClip{};
            const bool hadPreviousClip = SDL_GetRenderClipRect(renderer, &previousClip);

            SDL_Rect clipRect =
                hadPreviousClip
                    ? intersectRects(
                          previousClip,
                          nodeRect)
                    : nodeRect;

            if (clipRect.w <= 0 ||
                clipRect.h <= 0)
            {
                return;
            }

            SDL_SetRenderClipRect(
                renderer,
                &clipRect);
        }

        node.draw(renderer);

        auto *panel =
            dynamic_cast<PanelNode *>(&node);

        if (!panel)
            return;

        std::vector<NodeId> snapshot;
        snapshot.reserve(panel->children_.size());

        for (const auto &child : panel->children_)
        {
            if (child)
                snapshot.push_back(child->getId());
        }

        for (NodeId childId : snapshot)
        {
            const auto index =
                findChildIndexById(
                    panel->children_,
                    childId);

            if (!index)
                continue;

            Node *child =
                panel->children_[*index].get();

            if (!child ||
                child->getId() != childId ||
                child->parent_ != panel)
            {
                continue;
            }

            drawSubtree(*child, renderer);
        }
    }

    void NodeTree::draw(
        SDL_Renderer *renderer,
        std::optional<NodeId> topModalId)
    {
        if (!renderer)
            return;

        {
            ScopedMutationGuard guard(*this);

            forEachRoot(
                [this, renderer](Node &root)
                {
                    drawSubtree(root, renderer);
                    return false;
                });

            forEachOverlay(
                [this, renderer, topModalId](Node &overlay)
                {
                    if (topModalId &&
                        overlay.getId() == *topModalId)
                    {
                        return false;
                    }

                    drawSubtree(overlay, renderer);
                    return false;
                });

            if (topModalId)
            {
                if (Node *topModal =
                        findNode(*topModalId))
                {
                    drawSubtree(
                        *topModal,
                        renderer);
                }
            }
        }

        flushMutationQueue();
    }

    Node *NodeTree::hitTestSubtree(
        Node &node,
        float x,
        float y)
    {
        if (!node.isVisible() ||
            !node.isEnabled())
        {
            return nullptr;
        }

        Node *selfHit =
            node.hitTest(x, y);

        if (node.getClipToBounds())
        {
            const LayoutPosition position = node.getActualPosition();
            const LayoutSize size = node.getActualSize();

            if (x < position.x ||
                y < position.y ||
                x >= position.x + size.width ||
                y >= position.y + size.height)
            {
                return nullptr;
            }
        }

        auto *panel = dynamic_cast<PanelNode *>(&node);

        if (panel)
        {
            for (auto it = panel->children_.rbegin();
                 it != panel->children_.rend();
                 ++it)
            {
                if (!*it)
                    continue;

                Node *child = it->get();

                if (child->parent_ != panel)
                    continue;

                if (Node *target =
                        hitTestSubtree(
                            *child,
                            x,
                            y))
                {
                    return target;
                }
            }
        }

        return selfHit;
    }

    Node *NodeTree::hitTest(
        float x,
        float y,
        const Node *modalRoot)
    {
        ScopedMutationGuard guard(*this);

        if (modalRoot)
        {
            Node *liveModal =
                findNode(modalRoot->getId());

            if (liveModal != modalRoot)
                return nullptr;

            return hitTestSubtree(
                *liveModal,
                x,
                y);
        }

        for (size_t i = overlays_.size(); i > 0; --i)
        {
            Node *overlay =
                overlays_[i - 1].get();

            if (!overlay)
                continue;

            if (Node *target =
                    hitTestSubtree(
                        *overlay,
                        x,
                        y))
            {
                return target;
            }
        }

        for (size_t i = roots_.size(); i > 0; --i)
        {
            Node *root =
                roots_[i - 1].get();

            if (!root)
                continue;

            if (Node *target =
                    hitTestSubtree(
                        *root,
                        x,
                        y))
            {
                return target;
            }
        }

        return nullptr;
    }

    bool NodeTree::isDescendant(
        const Node *node,
        const Node *ancestor) const noexcept
    {
        if (!node || !ancestor)
            return false;

        if (findNode(node->getId()) != node ||
            findNode(ancestor->getId()) != ancestor)
        {
            return false;
        }

        const Node *current = node;

        while (current)
        {
            if (current == ancestor)
                return true;

            current = current->parent_;
        }

        return false;
    }

} // namespace ui
