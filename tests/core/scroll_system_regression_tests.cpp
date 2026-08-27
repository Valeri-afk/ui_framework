#include "ui_framework/ui_manager.hpp"
#include "ui_framework/node.hpp"
#include "ui_framework/panel_node.hpp"
#include "ui_framework/stack_panel_node.hpp"

#include <SDL3/SDL.h>

#include <cstdlib>
#include <iostream>
#include <memory>
#include <cmath>

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

    SDL_Event mouseWheel(float x, float y, float deltaX, float deltaY)
    {
        SDL_Event event{};
        event.type = SDL_EVENT_MOUSE_WHEEL;
        event.wheel.mouse_x = x;
        event.wheel.mouse_y = y;
        event.wheel.x = deltaX;
        event.wheel.y = deltaY;
        return event;
    }

    SDL_Event mouseDown(float x, float y)
    {
        SDL_Event event{};
        event.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
        event.button.x = x;
        event.button.y = y;
        event.button.button = SDL_BUTTON_LEFT;
        return event;
    }

    std::unique_ptr<ui::StackPanelNode> makeVerticalPanel(float width = 100.0f, float height = 100.0f)
    {
        auto panel = std::make_unique<ui::StackPanelNode>();
        panel->setSize(ui::LayoutSizeValue::fixed(width, height));
        panel->setOrientation(ui::StackPanelNode::Orientation::Vertical);
        return panel;
    }

    void layoutAndSync(ui::UIManager &manager, ui::Node &node)
    {
        manager.invalidateLayout(node);
        manager.render(nullptr);
    }

    void test_only_panels_can_be_scroll_containers()
    {
        ui::UIManager manager;
        auto leaf = std::make_unique<ui::Node>();
        ui::Node *node = manager.addRoot(std::move(leaf));

        expect(node != nullptr, "root node must attach");
        expect(!manager.enableScrolling(*node), "leaf Node must not become a scroll container");
        expect(!manager.isScrollingEnabled(*node), "failed registration must leave leaf unregistered");
    }

    void test_unmounted_panel_cannot_be_registered()
    {
        ui::UIManager manager;
        auto panel = makeVerticalPanel();
        expect(!manager.enableScrolling(*panel),
               "a panel outside the manager tree must not become a scroll container");
    }

    void test_scroll_registration_does_not_change_clipping()
    {
        ui::UIManager manager;
        auto panel = makeVerticalPanel();
        panel->setClipToBounds(false);
        ui::Node *root = manager.addRoot(std::move(panel));

        expect(manager.enableScrolling(*root), "attached panel must register for scrolling");
        expect(!root->getClipToBounds(),
               "scroll registration must not take ownership of the node's clipping state");
        expect(manager.disableScrolling(*root), "registered panel must unregister cleanly");
        expect(!root->getClipToBounds(),
               "unregister must not modify the node's clipping state");
    }

    void test_preexisting_clipping_survives_scroll_lifecycle()
    {
        ui::UIManager manager;
        auto panel = makeVerticalPanel();
        panel->setClipToBounds(true);
        ui::Node *root = manager.addRoot(std::move(panel));

        expect(manager.enableScrolling(*root), "attached panel must register");
        expect(root->getClipToBounds(), "existing clipping must remain enabled while scrolling");
        expect(manager.disableScrolling(*root), "registered panel must unregister");
        expect(root->getClipToBounds(), "existing clipping must remain enabled after unregister");
    }

    void test_content_extent_and_max_offset_are_derived_from_layout()
    {
        ui::UIManager manager;
        auto panel = makeVerticalPanel();
        panel->setSize(ui::LayoutSizeValue::fixed(100.0f, 100.0f));

        auto first = std::make_unique<ui::Node>();
        first->setSize(ui::LayoutSizeValue::fixed(100.0f, 80.0f));
        auto second = std::make_unique<ui::Node>();
        second->setSize(ui::LayoutSizeValue::fixed(100.0f, 80.0f));
        panel->addChild(std::move(first), 0);
        panel->addChild(std::move(second), 1);

        ui::Node *root = manager.addRoot(std::move(panel));
        expect(manager.enableScrolling(*root), "PanelNode must register as a scroll container");
        layoutAndSync(manager, *root);

        const ui::ScrollOffset maximum = manager.getMaximumScrollOffset(*root);
        expect(near(maximum.y, 60.0f), "content extent must produce a 60px vertical scroll range");
        expect(near(maximum.x, 0.0f), "content must not create horizontal scroll range");
    }

    void test_offset_is_clamped_against_current_geometry()
    {
        ui::UIManager manager;
        auto panel = makeVerticalPanel();
        auto child = std::make_unique<ui::Node>();
        child->setSize(ui::LayoutSizeValue::fixed(100.0f, 200.0f));
        panel->addChild(std::move(child), 0);

        ui::Node *root = manager.addRoot(std::move(panel));
        expect(manager.enableScrolling(*root), "panel must register for scrolling");
        layoutAndSync(manager, *root);

        expect(manager.setScrollOffset(*root, {0.0f, 500.0f}),
               "setScrollOffset must succeed for a registered panel");
        expect(near(manager.getScrollOffset(*root).y, 100.0f),
               "scroll offset must clamp to content range");
    }

    void test_scroll_does_not_move_container_itself()
    {
        ui::UIManager manager;
        auto panel = makeVerticalPanel();
        auto child = std::make_unique<ui::Node>();
        child->setSize(ui::LayoutSizeValue::fixed(100.0f, 200.0f));
        panel->addChild(std::move(child), 0);

        ui::Node *root = manager.addRoot(std::move(panel));
        expect(manager.enableScrolling(*root), "panel must register");
        layoutAndSync(manager, *root);

        expect(manager.setScrollOffset(*root, {0.0f, 50.0f}), "offset change must succeed");
        expect(root->getActualPosition() == ui::LayoutPosition{},
               "scrolling must not transform the scroll container itself");
    }

    void test_scrolled_content_remains_hittable_inside_viewport()
    {
        ui::UIManager manager;
        auto panel = makeVerticalPanel();
        panel->setClipToBounds(true);
        auto child = std::make_unique<ui::Node>();
        child->setSize(ui::LayoutSizeValue::fixed(100.0f, 200.0f));
        ui::Node *childPtr = child.get();
        panel->addChild(std::move(child), 0);

        ui::Node *root = manager.addRoot(std::move(panel));
        expect(manager.enableScrolling(*root), "panel must register");
        layoutAndSync(manager, *root);
        expect(manager.setScrollOffset(*root, {0.0f, 80.0f}), "offset change must succeed");

        int clicks = 0;
        childPtr->setFocusable(true);
        childPtr->setCapturable(true);
        childPtr->on<ui::MouseDownEvent>([&](ui::MouseDownEvent &, ui::Node &)
        {
            ++clicks;
        });

        manager.processEvent(mouseDown(50.0f, 10.0f));

        expect(clicks == 1, "hit-testing must follow the transformed scrolled content");
    }

    void test_scroll_viewport_blocks_hits_outside_container()
    {
        ui::UIManager manager;
        auto panel = makeVerticalPanel();
        panel->setClipToBounds(true);
        auto child = std::make_unique<ui::Node>();
        child->setSize(ui::LayoutSizeValue::fixed(100.0f, 200.0f));
        ui::Node *childPtr = child.get();
        panel->addChild(std::move(child), 0);

        ui::Node *root = manager.addRoot(std::move(panel));
        expect(manager.enableScrolling(*root), "panel must register");
        layoutAndSync(manager, *root);
        expect(manager.setScrollOffset(*root, {0.0f, 80.0f}), "offset change must succeed");

        int clicks = 0;
        childPtr->on<ui::MouseDownEvent>([&](ui::MouseDownEvent &, ui::Node &)
        {
            ++clicks;
        });

        manager.processEvent(mouseDown(50.0f, 120.0f));

        expect(clicks == 0,
               "scroll viewport clipping must prevent hits on overflow outside the container bounds");
    }

    void test_second_wheel_after_scroll_uses_same_scroll_container()
    {
        ui::UIManager manager;
        auto panel = makeVerticalPanel();
        auto child = std::make_unique<ui::Node>();
        child->setSize(ui::LayoutSizeValue::fixed(100.0f, 300.0f));
        panel->addChild(std::move(child), 0);

        ui::Node *root = manager.addRoot(std::move(panel));
        expect(manager.enableScrolling(*root), "panel must register");
        layoutAndSync(manager, *root);

        manager.processEvent(mouseWheel(50.0f, 50.0f, 0.0f, 40.0f));
        const float first = manager.getScrollOffset(*root).y;
        manager.processEvent(mouseWheel(50.0f, 50.0f, 0.0f, 40.0f));
        const float second = manager.getScrollOffset(*root).y;

        expect(near(first, 40.0f), "first wheel must scroll the container");
        expect(near(second, 80.0f),
               "a second wheel at the same viewport must continue scrolling the same container");
    }

    void test_nested_wheel_chains_to_outer_and_preserves_viewports()
    {
        ui::UIManager manager;
        auto outer = makeVerticalPanel(200.0f, 100.0f);

        auto inner = makeVerticalPanel(100.0f, 50.0f);
        auto innerChild = std::make_unique<ui::Node>();
        innerChild->setSize(ui::LayoutSizeValue::fixed(100.0f, 150.0f));
        inner->addChild(std::move(innerChild), 0);

        auto outerChild = std::make_unique<ui::Node>();
        outerChild->setSize(ui::LayoutSizeValue::fixed(200.0f, 100.0f));

        ui::Node *innerRaw = outer->addChild(std::move(inner), 0);
        outer->addChild(std::move(outerChild), 1);
        ui::Node *outerRaw = manager.addRoot(std::move(outer));

        expect(manager.enableScrolling(*outerRaw), "outer panel must register");
        expect(manager.enableScrolling(*innerRaw), "inner panel must register");
        layoutAndSync(manager, *outerRaw);

        const ui::LayoutPosition outerPositionBefore = outerRaw->getActualPosition();
        const ui::LayoutPosition innerPositionBefore = innerRaw->getActualPosition();

        manager.processEvent(mouseWheel(10.0f, 10.0f, 0.0f, 200.0f));

        expect(manager.getScrollOffset(*innerRaw).y > 0.0f,
               "wheel must scroll the nearest inner container first");
        expect(manager.getScrollOffset(*outerRaw).y > 0.0f,
               "remaining wheel delta must chain to the outer container");
        expect(outerRaw->getActualPosition() == outerPositionBefore,
               "outer scroll container viewport must remain stationary");
        expect(innerRaw->getActualPosition() == innerPositionBefore,
               "inner scroll container viewport must remain stationary relative to its parent");
    }

    void test_nested_scroll_container_viewport_contributes_but_inner_content_does_not()
    {
        ui::UIManager manager;
        auto outer = makeVerticalPanel(100.0f, 100.0f);
        auto inner = makeVerticalPanel(100.0f, 40.0f);

        auto innerChild = std::make_unique<ui::Node>();
        innerChild->setSize(ui::LayoutSizeValue::fixed(100.0f, 300.0f));
        inner->addChild(std::move(innerChild), 0);

        outer->addChild(std::move(inner), 0);
        ui::Node *root = manager.addRoot(std::move(outer));
        ui::Node *innerPtr = root->getVisibleChild(0);
        (void)innerPtr;

        expect(manager.enableScrolling(*root), "outer panel must register");
        expect(manager.enableScrolling(*innerPtr), "inner panel must register");
        layoutAndSync(manager, *root);

        const ui::ScrollOffset maximum = manager.getMaximumScrollOffset(*root);
        expect(near(maximum.y, 0.0f),
               "an inner scroll container's internal content must not expand the outer content extent");
    }

    void test_geometry_change_reclamps_scroll_offset()
    {
        ui::UIManager manager;
        auto panel = makeVerticalPanel();
        auto child = std::make_unique<ui::Node>();
        child->setSize(ui::LayoutSizeValue::fixed(100.0f, 300.0f));
        ui::Node *childPtr = child.get();
        panel->addChild(std::move(child), 0);

        ui::Node *root = manager.addRoot(std::move(panel));
        expect(manager.enableScrolling(*root), "panel must register");
        layoutAndSync(manager, *root);
        expect(manager.setScrollOffset(*root, {0.0f, 200.0f}), "initial scroll must succeed");

        childPtr->setSize(ui::LayoutSizeValue::fixed(100.0f, 120.0f));
        layoutAndSync(manager, *root);

        expect(near(manager.getScrollOffset(*root).y, 20.0f),
               "shrinking content must reclamp the stored scroll offset");
    }

    void test_removed_scroll_node_is_not_left_registered()
    {
        ui::UIManager manager;
        auto panel = makeVerticalPanel();
        ui::Node *root = manager.addRoot(std::move(panel));
        expect(manager.enableScrolling(*root), "panel must register");

        manager.removeRoot(root);
        manager.advanceTime(0.0f);
        manager.render(nullptr);

        expect(!manager.isScrollingEnabled(*root),
               "removed scroll node must not remain registered after synchronization");
        expect(manager.getScrollOffset(*root) == ui::ScrollOffset{},
               "removed scroll node must not retain stale offset state");
    }

    void test_disable_then_reenable_scroll_resets_runtime_state()
    {
        ui::UIManager manager;
        auto panel = makeVerticalPanel();
        auto child = std::make_unique<ui::Node>();
        child->setSize(ui::LayoutSizeValue::fixed(100.0f, 200.0f));
        panel->addChild(std::move(child), 0);

        ui::Node *root = manager.addRoot(std::move(panel));
        expect(manager.enableScrolling(*root), "panel must register");
        layoutAndSync(manager, *root);
        expect(manager.setScrollOffset(*root, {0.0f, 50.0f}), "offset must be settable");
        expect(manager.disableScrolling(*root), "panel must unregister");
        expect(manager.getScrollOffset(*root) == ui::ScrollOffset{},
               "unregister must remove stored offset state");
        expect(manager.enableScrolling(*root), "panel must be registerable again");
        layoutAndSync(manager, *root);
        expect(manager.getScrollOffset(*root) == ui::ScrollOffset{},
               "re-registration must start with a fresh zero offset");
    }
}

int main()
{
    try
    {
        test_only_panels_can_be_scroll_containers();
        test_unmounted_panel_cannot_be_registered();
        test_scroll_registration_does_not_change_clipping();
        test_preexisting_clipping_survives_scroll_lifecycle();
        test_content_extent_and_max_offset_are_derived_from_layout();
        test_offset_is_clamped_against_current_geometry();
        test_scroll_does_not_move_container_itself();
        test_scrolled_content_remains_hittable_inside_viewport();
        test_scroll_viewport_blocks_hits_outside_container();
        test_second_wheel_after_scroll_uses_same_scroll_container();
        test_nested_wheel_chains_to_outer_and_preserves_viewports();
        test_nested_scroll_container_viewport_contributes_but_inner_content_does_not();
        test_geometry_change_reclamps_scroll_offset();
        test_removed_scroll_node_is_not_left_registered();
        test_disable_then_reenable_scroll_resets_runtime_state();
    }
    catch (const TestFailure &failure)
    {
        std::cerr << "ScrollSystem regression tests failed: " << failure.message << '\n';
        return EXIT_FAILURE;
    }

    std::cout << "ScrollSystem regression tests passed\n";
    return EXIT_SUCCESS;
}
