#pragma once

#include <functional>

#include "ui_framework/types.hpp"

namespace ui
{
    class Node;

    struct MeasureContext
    {
        // A per-axis upper constraint for the content being measured.
        // A very large / infinite value means that the axis is unbounded.
        // The measured desired size is not required to fit inside this value.
        LayoutSize availableContentSize{};

        // Measures the child using the supplied content constraints and returns
        // the framework-resolved desired size. The framework owns recursive
        // traversal and the interpretation of its own layout properties.
        std::function<LayoutSize(Node&, const LayoutSize&)> measureChild;
    };

    struct ArrangeContext
    {
        LayoutPosition contentPosition{};
        LayoutSize contentSize{};

        // Returns the child's desired size produced by the Measure pass.
        std::function<LayoutSize(Node &)> desiredSize;

        // Assigns the allocation selected by the custom layout policy. The
        // framework remains responsible for final-size resolution and recursive
        // Arrange execution.
        std::function<void(Node &, const LayoutPosition &, const LayoutSize &)> arrangeChild;
    };
}
