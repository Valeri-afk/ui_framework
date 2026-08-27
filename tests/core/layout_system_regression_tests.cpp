#include "layout_system.hpp"
#include "node_tree.hpp"
#include "ui_framework/node.hpp"
#include "ui_framework/stack_panel_node.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <memory>

namespace
{
    struct TestFailure
    {
        const char *message;
    };

    void expect(bool condition, const char *message)
    {
        if (!condition)
            throw TestFailure{message};
    }

    bool near(float a, float b, float epsilon = 0.001f)
    {
        return std::fabs(a - b) <= epsilon;
    }

    struct LayoutFixture
    {
        ui::NodeTree tree;
        ui::LayoutSystem layout;

        explicit LayoutFixture(float width = 300.0f, float height = 300.0f)
        {
            layout.setViewportSize({width, height});
        }

        void process()
        {
            layout.requestFullLayout(tree);
            layout.processLayoutQueue(tree);
        }
    };

    class MeasuringNode final : public ui::Node
    {
    public:
        explicit MeasuringNode(ui::LayoutSize intrinsic) : intrinsic_(intrinsic) {}

        ui::LayoutSize lastAvailable() const noexcept
        {
            return lastAvailable_;
        }

    protected:
        ui::LayoutSize measureContent(const ui::LayoutSize &availableContent) const override
        {
            lastAvailable_ = availableContent;
            return intrinsic_;
        }

    private:
        ui::LayoutSize intrinsic_{};
        mutable ui::LayoutSize lastAvailable_{};
    };

    void test_border_box_padding_and_border_conversion()
    {
        LayoutFixture f;
        auto panel = std::make_unique<ui::StackPanelNode>();
        panel->setSize(ui::LayoutSizeValue::fixed(100.0f, 100.0f));
        panel->setPadding({10.0f, 10.0f, 10.0f, 10.0f});
        panel->setBorder({2.0f, 2.0f, 2.0f, 2.0f});

        auto child = std::make_unique<ui::Node>();
        child->setSize(ui::LayoutSizeValue::fixed(20.0f, 20.0f));
        ui::Node *childPtr = child.get();
        panel->addChild(std::move(child), 0);

        ui::Node *root = f.tree.attachRoot(0, std::move(panel));
        f.process();

        expect(root->getActualSize() == ui::LayoutSize{100.0f, 100.0f},
               "fixed root size must remain a border-box size");
        expect(near(childPtr->getActualPosition().x, 12.0f) &&
               near(childPtr->getActualPosition().y, 12.0f),
               "child content position must account for border and padding");
    }

    void test_min_and_max_have_distinct_measurement_semantics()
    {
        LayoutFixture f;
        auto panel = std::make_unique<ui::StackPanelNode>();
        panel->setSize(ui::LayoutSizeValue::fixed(200.0f, 150.0f));

        auto minNode = std::make_unique<MeasuringNode>(ui::LayoutSize{80.0f, 20.0f});
        minNode->setMinSize({100.0f, 10.0f});
        MeasuringNode *minPtr = minNode.get();
        panel->addChild(std::move(minNode), 0);

        auto maxNode = std::make_unique<MeasuringNode>(ui::LayoutSize{120.0f, 20.0f});
        maxNode->setMaxSize({60.0f, 40.0f});
        MeasuringNode *maxPtr = maxNode.get();
        panel->addChild(std::move(maxNode), 1);

        f.tree.attachRoot(0, std::move(panel));
        f.process();

        expect(minPtr->lastAvailable().width >= 80.0f,
               "minimum size must not narrow intrinsic measurement proposal");
        expect(near(minPtr->getDesiredSize().width, 100.0f),
               "minimum size must constrain the committed desired width");

        expect(near(maxPtr->lastAvailable().width, 60.0f),
               "maximum size may narrow the measurement proposal");
        expect(near(maxPtr->getDesiredSize().width, 60.0f),
               "maximum size must constrain the committed desired width");
        expect(near(maxPtr->getActualSize().width, 60.0f),
               "maximum size must constrain final arranged width");
    }

    void test_stack_panel_main_alignment_and_gap()
    {
        LayoutFixture f;
        auto panel = std::make_unique<ui::StackPanelNode>();
        panel->setSize(ui::LayoutSizeValue::fixed(100.0f, 100.0f));
        panel->setOrientation(ui::StackPanelNode::Orientation::Vertical);
        panel->setGap(10.0f);
        panel->setMainAlignment(ui::MainAxisAlignment::CENTER);

        auto first = std::make_unique<ui::Node>();
        first->setSize(ui::LayoutSizeValue::fixed(20.0f, 20.0f));
        ui::Node *firstPtr = first.get();
        panel->addChild(std::move(first), 0);

        auto second = std::make_unique<ui::Node>();
        second->setSize(ui::LayoutSizeValue::fixed(20.0f, 20.0f));
        ui::Node *secondPtr = second.get();
        panel->addChild(std::move(second), 1);

        f.tree.attachRoot(0, std::move(panel));
        f.process();

        expect(near(firstPtr->getActualPosition().y, 25.0f),
               "centered stack must apply leading main-axis free space");
        expect(near(secondPtr->getActualPosition().y, 55.0f),
               "stack gap must be preserved between centered children");
    }

    void test_cross_axis_stretch_respects_child_maximum()
    {
        LayoutFixture f;
        auto panel = std::make_unique<ui::StackPanelNode>();
        panel->setSize(ui::LayoutSizeValue::fixed(100.0f, 100.0f));
        panel->setOrientation(ui::StackPanelNode::Orientation::Vertical);
        panel->setCrossAlignment(ui::CrossAxisAlignment::STRETCH);

        auto child = std::make_unique<ui::Node>();
        child->setSize(ui::LayoutSizeValue::autoSize());
        child->setMaxWidth(40.0f);
        ui::Node *childPtr = child.get();
        panel->addChild(std::move(child), 0);

        f.tree.attachRoot(0, std::move(panel));
        f.process();

        expect(near(childPtr->getActualSize().width, 40.0f),
               "cross-axis stretch must still respect the child's maximum width");
    }

    void test_absolute_child_is_excluded_from_linear_flow()
    {
        LayoutFixture f;
        auto panel = std::make_unique<ui::StackPanelNode>();
        panel->setSize(ui::LayoutSizeValue::fixed(100.0f, 100.0f));
        panel->setOrientation(ui::StackPanelNode::Orientation::Vertical);

        auto flowChild = std::make_unique<ui::Node>();
        flowChild->setSize(ui::LayoutSizeValue::fixed(20.0f, 20.0f));
        ui::Node *flowPtr = flowChild.get();
        panel->addChild(std::move(flowChild), 0);

        auto absoluteChild = std::make_unique<ui::Node>();
        absoluteChild->setSize(ui::LayoutSizeValue::fixed(30.0f, 30.0f));
        absoluteChild->setPosition({40.0f, 50.0f});
        absoluteChild->setPositionMode(ui::PositionMode::Absolute);
        ui::Node *absolutePtr = absoluteChild.get();
        panel->addChild(std::move(absoluteChild), 1);

        f.tree.attachRoot(0, std::move(panel));
        f.process();

        expect(near(flowPtr->getActualPosition().y, 0.0f),
               "absolute children must not shift normal-flow children");
        expect(near(absolutePtr->getActualPosition().x, 40.0f) &&
               near(absolutePtr->getActualPosition().y, 50.0f),
               "absolute child must keep its explicit position in parent content coordinates");
    }

    void test_auto_size_does_not_mean_fill_parent()
    {
        LayoutFixture f;
        auto panel = std::make_unique<ui::StackPanelNode>();
        panel->setSize(ui::LayoutSizeValue::fixed(200.0f, 100.0f));
        panel->setCrossAlignment(ui::CrossAxisAlignment::START);

        auto child = std::make_unique<MeasuringNode>(ui::LayoutSize{30.0f, 20.0f});
        ui::Node *childPtr = child.get();
        panel->addChild(std::move(child), 0);

        f.tree.attachRoot(0, std::move(panel));
        f.process();

        expect(near(childPtr->getActualSize().width, 30.0f) &&
               near(childPtr->getActualSize().height, 20.0f),
               "auto size must preserve intrinsic desired size when cross-axis stretching is disabled");
    }

    void test_nested_clipping_uses_intersection_of_ancestor_bounds()
    {
        LayoutFixture f(200.0f, 200.0f);

        auto outer = std::make_unique<ui::PanelNode>();
        outer->setSize(ui::LayoutSizeValue::fixed(100.0f, 100.0f));
        outer->setClipToBounds(true);
        ui::PanelNode *outerPtr = outer.get();
        f.tree.attachRoot(0, std::move(outer));

        auto inner = std::make_unique<ui::PanelNode>();
        inner->setSize(ui::LayoutSizeValue::fixed(60.0f, 60.0f));
        inner->setPosition({40.0f, 40.0f});
        inner->setPositionMode(ui::PositionMode::Absolute);
        inner->setClipToBounds(true);
        ui::PanelNode *innerPtr = inner.get();
        f.tree.attachChild(*outerPtr, std::move(inner), 0);

        auto child = std::make_unique<ui::Node>();
        child->setSize(ui::LayoutSizeValue::fixed(80.0f, 80.0f));
        child->setPosition({20.0f, 20.0f});
        child->setPositionMode(ui::PositionMode::Absolute);
        ui::Node *childPtr = child.get();
        f.tree.attachChild(*innerPtr, std::move(child), 0);

        f.process();

        expect(f.tree.hitTest(60.0f, 60.0f) == childPtr,
               "point inside both clipping bounds must reach the descendant");
        expect(f.tree.hitTest(110.0f, 60.0f) == nullptr,
               "outer clipping boundary must block the descendant");
        expect(f.tree.hitTest(60.0f, 110.0f) == nullptr,
               "outer clipping boundary must block the descendant vertically");
        expect(f.tree.hitTest(95.0f, 95.0f) == childPtr,
               "point near the far edge must remain hittable while inside both clip intersections");
    }

    void test_relayout_commits_changed_geometry_only_after_processing()
    {
        LayoutFixture f;
        auto root = std::make_unique<ui::StackPanelNode>();
        root->setSize(ui::LayoutSizeValue::fixed(100.0f, 100.0f));

        auto child = std::make_unique<ui::Node>();
        child->setSize(ui::LayoutSizeValue::fixed(20.0f, 20.0f));
        ui::Node *childPtr = child.get();
        root->addChild(std::move(child), 0);

        f.tree.attachRoot(0, std::move(root));
        f.process();
        expect(childPtr->getActualSize() == ui::LayoutSize{20.0f, 20.0f},
               "initial geometry must be committed");

        childPtr->setSize(ui::LayoutSizeValue::fixed(40.0f, 30.0f));
        expect(childPtr->getActualSize() == ui::LayoutSize{20.0f, 20.0f},
               "setSize must not synchronously rewrite committed layout geometry");

        f.layout.requestFullLayout(f.tree);
        f.layout.processLayoutQueue(f.tree);

        expect(childPtr->getActualSize() == ui::LayoutSize{40.0f, 30.0f},
               "next layout pass must commit the changed geometry");
    }
}

int main()
{
    try
    {
        test_border_box_padding_and_border_conversion();
        test_min_and_max_have_distinct_measurement_semantics();
        test_stack_panel_main_alignment_and_gap();
        test_cross_axis_stretch_respects_child_maximum();
        test_absolute_child_is_excluded_from_linear_flow();
        test_auto_size_does_not_mean_fill_parent();
        test_nested_clipping_uses_intersection_of_ancestor_bounds();
        test_relayout_commits_changed_geometry_only_after_processing();
    }
    catch (const TestFailure &failure)
    {
        std::cerr << "LayoutSystem regression tests failed: " << failure.message << '\n';
        return EXIT_FAILURE;
    }

    std::cout << "LayoutSystem regression tests passed\n";
    return EXIT_SUCCESS;
}
