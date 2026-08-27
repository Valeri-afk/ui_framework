#include "event_dispatcher.hpp"
#include "node_tree.hpp"
#include "panel_node.hpp"
#include "ui_framework/components/checkbox.hpp"

#include <cassert>
#include <memory>

namespace
{
    struct TestEvent : ui::UIEvent
    {
        int value = 0;
    };

    class EventProbe final : public ui::PanelNode
    {
    public:
        void fire(int value = 0)
        {
            TestEvent event;
            event.value = value;
            emit(event);
        }
    };

    void test_multiple_same_type_handlers_all_run()
    {
        EventProbe node;
        int firstCalls = 0;
        int secondCalls = 0;

        node.on<TestEvent>([&](TestEvent &event, ui::Node &target)
        {
            assert(&target == &node);
            assert(event.value == 42);
            ++firstCalls;
        });

        node.on<TestEvent>([&](TestEvent &event, ui::Node &target)
        {
            assert(&target == &node);
            assert(event.value == 42);
            ++secondCalls;
        });

        node.fire(42);

        assert(firstCalls == 1);
        assert(secondCalls == 1);
    }

    void test_stop_propagation_does_not_cancel_same_target_handler()
    {
        ui::NodeTree tree;
        auto parent = std::make_unique<EventProbe>();
        auto child = std::make_unique<EventProbe>();

        EventProbe *parentPtr = parent.get();
        EventProbe *childPtr = child.get();

        parentPtr->setSize(ui::LayoutSizeValue::fixed(100.0f, 100.0f));
        childPtr->setSize(ui::LayoutSizeValue::fixed(50.0f, 50.0f));

        tree.attachRoot(0, std::move(parent));
        parentPtr->addChild(std::move(child), 0);

        int firstCalls = 0;
        int secondCalls = 0;
        int parentCalls = 0;

        childPtr->on<TestEvent>([&](TestEvent &, ui::Node &)
        {
            ++firstCalls;
        });

        childPtr->on<TestEvent>([&](TestEvent &event, ui::Node &)
        {
            ++secondCalls;
            event.stopPropagation();
        });

        parentPtr->on<TestEvent>([&](TestEvent &, ui::Node &)
        {
            ++parentCalls;
        });

        TestEvent event;
        ui::EventDispatcher::dispatch(tree, childPtr, event, false, true);

        assert(firstCalls == 1);
        assert(secondCalls == 1);
        assert(parentCalls == 0);
    }

    void test_handler_removal_during_dispatch_uses_snapshot()
    {
        EventProbe node;
        int firstCalls = 0;
        int secondCalls = 0;
        int thirdCalls = 0;
        ui::Node::EventHandlerId secondId = 0;

        node.on<TestEvent>([&](TestEvent &, ui::Node &)
        {
            ++firstCalls;
            node.removeEventHandler<TestEvent>(secondId);
        });

        secondId = node.on<TestEvent>([&](TestEvent &, ui::Node &)
        {
            ++secondCalls;
        });

        node.on<TestEvent>([&](TestEvent &, ui::Node &)
        {
            ++thirdCalls;
        });

        node.fire();
        assert(firstCalls == 1);
        assert(secondCalls == 1);
        assert(thirdCalls == 1);

        node.fire();
        assert(firstCalls == 2);
        assert(secondCalls == 1);
        assert(thirdCalls == 2);
    }

    void test_checkbox_emits_semantic_event_without_input_system()
    {
        ui::Checkbox checkbox;
        int calls = 0;
        bool observedChecked = false;

        checkbox.on<ui::CheckboxToggledEvent>(
            [&](ui::CheckboxToggledEvent &event, ui::Node &node)
            {
                ++calls;
                observedChecked = event.checked && static_cast<ui::Checkbox &>(node).isChecked();
            });

        checkbox.toggle();

        assert(checkbox.isChecked());
        assert(calls == 1);
        assert(observedChecked);
    }
}

int main()
{
    test_multiple_same_type_handlers_all_run();
    test_stop_propagation_does_not_cancel_same_target_handler();
    test_handler_removal_during_dispatch_uses_snapshot();
    test_checkbox_emits_semantic_event_without_input_system();
    return 0;
}
