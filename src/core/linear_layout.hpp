#pragma once

#include "ui_framework/types.hpp"
#include <functional>

namespace ui
{
    class Node;
    class StackPanelNode;

    namespace internal
    {
        struct LinearMeasureContext
        {
            LayoutSize availableSize{};
            std::function<LayoutSize(Node&, const LayoutSize&)> measureChild;
        };

        struct LinearArrangeContext
        {
            LayoutPosition contentPosition{};
            LayoutSize contentSize{};
            std::function<void(Node &, const LayoutPosition &, const LayoutSize &)> placeChild;
        };

        LayoutSize measureLinearPanel(
            const StackPanelNode &panel,
            const LinearMeasureContext &ctx);

        void arrangeLinearPanel(
            StackPanelNode &panel,
            LinearArrangeContext &ctx);
    }
}
