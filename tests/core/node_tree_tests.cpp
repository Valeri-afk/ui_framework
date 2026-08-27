#include "node_tree.hpp"
#include "ui_framework/panel_node.hpp"

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace
{
    struct TestFailure { std::string message; };

    void expect(bool condition, const char *message)
    {
        if (!condition)
            throw TestFailure{message};
    }

    std::unique_ptr<ui::Node> makeNode()
    {
        return std::make_unique<ui::Node>();
    }

    void test_root_lifecycle_and_registration()
    {
        ui::NodeTree tree;
        auto node = makeNode();
        const ui::Node::Id id = node->getId();
        ui::Node *root = tree.attachRoot(0, std::move(node));

        expect(root != nullptr, "attachRoot must return the live root");
        expect(tree.rootsCount() == 1, "root must be registered");
        expect(tree.findNode(id) == root, "root must be discoverable while live");
        expect(tree.isRoot(root), "attached node must be recognized as root");

        tree.removeRoot(root);

        expect(tree.rootsCount() == 0, "removed root must leave root storage");
        expect(tree.findNode(id) == nullptr, "removed root must be unregistered");
        expect(!tree.isNodeLive(id), "removed root must not remain live");
    }

    void test_overlay_lifecycle_is_separate_from_roots()
    {
        ui::NodeTree tree;
        ui::Node *root = tree.attachRoot(0, makeNode());
        ui::Node *overlay = tree.attachOverlay(0, makeNode());
        const ui::Node::Id overlayId = overlay->getId();

        expect(tree.rootsCount() == 1, "root count must exclude overlays");
        expect(tree.overlaysCount() == 1, "overlay count must be tracked separately");
        expect(tree.isRoot(root), "root must remain a root");
        expect(tree.isOverlay(overlay), "overlay must be recognized as overlay");

        tree.removeOverlay(overlay);

        expect(tree.overlaysCount() == 0, "removed overlay must leave overlay storage");
        expect(tree.findNode(overlayId) == nullptr, "removed overlay must be unregistered");
        expect(tree.findNode(root->getId()) == root, "removing overlay must not affect root");
    }

    void test_traversal_uses_stable_snapshot_for_removal()
    {
        ui::NodeTree tree;
        ui::Node *first = tree.attachRoot(0, makeNode());
        ui::Node *second = tree.attachRoot(1, makeNode());
        const ui::Node::Id firstId = first->getId();
        int callbacks = 0;

        tree.forEachRoot([&](ui::Node &node)
        {
            ++callbacks;
            if (&node == first)
                tree.removeRoot(&node);
            return false;
        });

        expect(callbacks == 2, "active traversal must use a stable snapshot");
        expect(tree.findNode(firstId) == nullptr, "deferred removal must flush after traversal");
        expect(tree.rootsCount() == 1, "one root must remain");
        expect(tree.getRoot(0) == second, "remaining root must remain accessible");
    }

    void test_traversal_does_not_visit_new_root_until_next_traversal()
    {
        ui::NodeTree tree;
        ui::Node *first = tree.attachRoot(0, makeNode());
        tree.attachRoot(1, makeNode());
        int callbacks = 0;
        ui::Node *added = nullptr;

        tree.forEachRoot([&](ui::Node &node)
        {
            ++callbacks;
            if (&node == first)
                added = tree.attachRoot(2, makeNode());
            return false;
        });

        expect(added == nullptr, "root creation during traversal must be deferred");
        expect(callbacks == 2, "new root must not enter the active traversal");
        expect(tree.rootsCount() == 3, "deferred root must be committed after traversal");
    }

    void test_reverse_traversal_order()
    {
        ui::NodeTree tree;
        ui::Node *first = tree.attachRoot(0, makeNode());
        ui::Node *second = tree.attachRoot(1, makeNode());
        ui::Node *third = tree.attachRoot(2, makeNode());
        std::vector<ui::Node::Id> visited;

        tree.rForEachRoot([&](ui::Node &node)
        {
            visited.push_back(node.getId());
            return false;
        });

        expect(visited.size() == 3, "reverse traversal must visit every root");
        expect(visited[0] == third->getId(), "reverse traversal must start at the last root");
        expect(visited[1] == second->getId(), "reverse traversal must visit middle root second");
        expect(visited[2] == first->getId(), "reverse traversal must end at the first root");
    }

    void test_advance_time_is_safe_without_nodes()
    {
        ui::NodeTree tree;
        tree.advanceTime(1.0f / 60.0f);
        expect(tree.rootsCount() == 0, "time advancement must not require a node traversal");
    }

    void test_advance_time_is_independent_from_render_traversal()
    {
        ui::NodeTree tree;
        ui::Node *root = tree.attachRoot(0, makeNode());
        const ui::Node::Id id = root->getId();

        tree.advanceTime(0.5f);

        expect(tree.findNode(id) == root, "advanceTime must not mutate tree structure by itself");
        expect(tree.rootsCount() == 1, "advanceTime must not perform a render traversal");
    }
}

int main()
{
    try
    {
        test_root_lifecycle_and_registration();
        test_overlay_lifecycle_is_separate_from_roots();
        test_traversal_uses_stable_snapshot_for_removal();
        test_traversal_does_not_visit_new_root_until_next_traversal();
        test_reverse_traversal_order();
        test_advance_time_is_safe_without_nodes();
        test_advance_time_is_independent_from_render_traversal();
    }
    catch (const TestFailure &failure)
    {
        std::cerr << "NodeTree tests failed: " << failure.message << '\n';
        return EXIT_FAILURE;
    }

    std::cout << "NodeTree tests passed\n";
    return EXIT_SUCCESS;
}
