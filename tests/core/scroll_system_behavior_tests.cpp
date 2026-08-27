#include "ui_framework/ui_manager.hpp"
#include "ui_framework/node.hpp"
#include "ui_framework/panel_node.hpp"
#include "ui_framework/stack_panel_node.hpp"

#include <SDL3/SDL.h>

#include <cstdlib>
#include <iostream>
#include <cmath>
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

    SDL_Event mouseMove(float x, float y)
    {
        SDL_Event event{};
        event.type = SDL_EVENT_MOUSE_MOTION;
        event.motion.x = x;
        event.motion.y = y;
        return event;
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

    std::unique_ptr<ui::StackPanelNode> makeVerticalPanel(float width, float height)
    {
        auto panel = std::make_unique<ui::StackPanelNode>();
        panel->setSize(ui::LayoutSizeValue::fixed(width, height));
        panel->setOrientation(ui::StackPanelNode::Orientation::Vertical);
        return panel;
    }

    void prepare(ui::UIManager &manager, ui::Node &node)
    {
        manager.invalidateLayout(node);
        manager.render(nullptr);
    }

    void test_boundary_chaining_in_both_directions()
    {
        ui::UIManager manager;
        auto outer = makeVerticalPanel(100.0f, 100.0f);

        auto inner = makeVerticalPanel(100.0f, 50.0f);
        auto innerContent = std::make_unique<ui::Node>();
        innerContent->setSize(ui::LayoutSizeValue::fixed(100.0f, 150.0f));
        inner->addChild(std::move(innerContent), 0);

        auto outerTail = std::make_unique<ui::Node>();
        outerTail->setSize(ui::LayoutSizeValue::fixed(100.0f, 100.0f));

        ui::Node *innerRaw = outer->addChild(std::move(inner), 0);
        outer->addChild(std::move(outerTail), 1);
        ui::Node *outerRaw = manager.addRoot(std::move(outer));

        expect(manager.enableScrolling(*outerRaw), "outer panel must register");
        expect(manager.enableScrolling(*innerRaw), "inner panel must register");
        prepare(manager, *outerRaw);

        expect(manager.setScrollOffset(*innerRaw, {0.0f, 100.0f}), "inner must reach bottom boundary");
        expect(manager.setScrollOffset(*outerRaw, {0.0f, 0.0f}), "outer must start at top boundary");

        manager.processEvent(mouseWheel(10.0f, 10.0f, 0.0f, 120.0f));
        expect(near(manager.getScrollOffset(*innerRaw).y, 100.0f),
               "downward wheel at inner bottom must leave inner at its boundary");
        expect(manager.getScrollOffset(*outerRaw).y > 0.0f,
               "downward wheel at inner bottom must chain remaining delta to outer");

        const float outerAfterDown = manager.getScrollOffset(*outerRaw).y;
        expect(manager.setScrollOffset(*innerRaw, {0.0f, 0.0f}), "inner must return to top boundary");
        expect(manager.setScrollOffset(*outerRaw, {0.0f, outerAfterDown}), "outer must keep its scrolled position");

        manager.processEvent(mouseWheel(10.0f, 10.0f, 0.0f, -40.0f));
        expect(near(manager.getScrollOffset(*innerRaw).y, 0.0f),
               "upward wheel at inner top must leave inner at its boundary");
        expect(manager.getScrollOffset(*outerRaw).y < outerAfterDown,
               "upward wheel at inner top must chain remaining delta to outer");
    }

    void test_hover_is_refreshed_after_scroll()
    {
        ui::UIManager manager;
        auto panel = makeVerticalPanel(100.0f, 100.0f);

        auto spacer = std::make_unique<ui::Node>();
        spacer->setSize(ui::LayoutSizeValue::fixed(100.0f, 150.0f));
        auto target = std::make_unique<ui::Node>();
        target->setSize(ui::LayoutSizeValue::fixed(100.0f, 50.0f));
        ui::Node *targetRaw = target.get();

        panel->addChild(std::move(spacer), 0);
        panel->addChild(std::move(target), 1);
        ui::Node *root = manager.addRoot(std::move(panel));
        expect(manager.enableScrolling(*root), "panel must register");
        prepare(manager, *root);

        int entered = 0;
        targetRaw->on<ui::MouseEnterEvent>([&](ui::MouseEnterEvent &, ui::Node &)
        {
            ++entered;
        });

        manager.processEvent(mouseMove(10.0f, 75.0f));
        expect(entered == 0, "target must start outside the pointer position");

        manager.processEvent(mouseWheel(10.0f, 75.0f, 0.0f, 100.0f));
        expect(entered == 1,
               "scrolling under a stationary pointer must refresh hover without synthesizing mouse motion");
    }

    void test_modal_wheel_does_not_scroll_lower_content()
    {
        ui::UIManager manager;
        auto lower = makeVerticalPanel(100.0f, 100.0f);
        auto lowerContent = std::make_unique<ui::Node>();
        lowerContent->setSize(ui::LayoutSizeValue::fixed(100.0f, 250.0f));
        lower->addChild(std::move(lowerContent), 0);
        ui::Node *lowerRaw = manager.addRoot(std::move(lower));
        expect(manager.enableScrolling(*lowerRaw), "lower panel must register");
        prepare(manager, *lowerRaw);

        auto modal = makeVerticalPanel(100.0f, 100.0f);
        ui::Node *modalRaw = manager.addOverlay(std::move(modal));
        expect(modalRaw != nullptr, "modal overlay must attach");
        expect(manager.showModal(*modalRaw), "overlay panel must become modal");
        prepare(manager, *modalRaw);

        manager.processEvent(mouseWheel(50.0f, 50.0f, 0.0f, 80.0f));

        expect(near(manager.getScrollOffset(*lowerRaw).y, 0.0f),
               "wheel inside a modal must not route to a lower scroll container");
    }

    void test_viewport_resize_reclamps_offset()
    {
        ui::UIManager manager;
        auto panel = makeVerticalPanel(100.0f, 100.0f);
        auto content = std::make_unique<ui::Node>();
        content->setSize(ui::LayoutSizeValue::fixed(100.0f, 200.0f));
        panel->addChild(std::move(content), 0);

        ui::Node *root = manager.addRoot(std::move(panel));
        expect(manager.enableScrolling(*root), "panel must register");
        prepare(manager, *root);
        expect(manager.setScrollOffset(*root, {0.0f, 100.0f}), "offset must reach old maximum");

        root->setSize(ui::LayoutSizeValue::fixed(100.0f, 150.0f));
        prepare(manager, *root);

        expect(near(manager.getMaximumScrollOffset(*root).y, 50.0f),
               "viewport growth must reduce maximum scroll offset");
        expect(near(manager.getScrollOffset(*root).y, 50.0f),
               "viewport growth must reclamp an existing offset");
    }
}

int main()
{
    try
    {
        test_boundary_chaining_in_both_directions();
        test_hover_is_refreshed_after_scroll();
        test_modal_wheel_does_not_scroll_lower_content();
        test_viewport_resize_reclamps_offset();
    }
    catch (const TestFailure &failure)
    {
        std::cerr << "ScrollSystem behavior tests failed: " << failure.message << '\n';
        return EXIT_FAILURE;
    }

    std::cout << "ScrollSystem behavior tests passed\n";
    return EXIT_SUCCESS;
}
