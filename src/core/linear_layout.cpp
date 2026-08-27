#include "linear_layout.hpp"
#include "layout_constraints.hpp"
#include "ui_framework/stack_panel_node.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

namespace
{
    constexpr float kInfinity = std::numeric_limits<float>::max();

    float finiteOrZero(float value) noexcept
    {
        return std::isfinite(value) ? value : 0.0f;
    }

    float safeAdd(float a, float b) noexcept
    {
        if (!std::isfinite(a) || !std::isfinite(b))
            return kInfinity;
        const float result = a + b;
        return std::isfinite(result) ? result : kInfinity;
    }

    bool isVerticalOrientation(ui::StackPanelNode::Orientation orientation) noexcept
    {
        return orientation == ui::StackPanelNode::Orientation::Vertical;
    }

    float normalizedGap(float gap) noexcept
    {
        return std::isfinite(gap) && gap > 0.0f ? gap : 0.0f;
    }

    void accumulateContentSize(
        ui::LayoutSize &size,
        ui::LayoutSize childSize,
        bool vertical) noexcept
    {
        if (vertical)
        {
            size.width = std::max(finiteOrZero(size.width), finiteOrZero(childSize.width));
            size.height = safeAdd(finiteOrZero(size.height), finiteOrZero(childSize.height));
        }
        else
        {
            size.width = safeAdd(finiteOrZero(size.width), finiteOrZero(childSize.width));
            size.height = std::max(finiteOrZero(size.height), finiteOrZero(childSize.height));
        }
    }
}

namespace ui::internal
{
    LayoutSize measureLinearPanel(
        const StackPanelNode &panel,
        const LinearMeasureContext &context)
    {
        if (!context.measureChild)
            return {};

        const bool vertical = isVerticalOrientation(panel.getOrientation());
        const float gap = normalizedGap(panel.getGap());
        LayoutSize contentSize{};
        std::size_t flowChildCount = 0;

        for (std::size_t i = 0; i < panel.getChildCount(); ++i)
        {
            Node *child = const_cast<StackPanelNode &>(panel).getChild(i);
            if (!child || !child->isVisible())
                continue;

            if (child->getPositionMode() == PositionMode::Absolute)
                continue;

            LayoutSize childAvailable = context.availableSize;
            if (vertical)
                childAvailable.height = kInfinity;
            else
                childAvailable.width = kInfinity;

            const LayoutSize childSize = context.measureChild(*child, childAvailable);
            accumulateContentSize(contentSize, childSize, vertical);
            ++flowChildCount;
        }

        if (flowChildCount > 1)
        {
            const float totalGap = gap * static_cast<float>(flowChildCount - 1);
            if (vertical)
                contentSize.height = safeAdd(contentSize.height, totalGap);
            else
                contentSize.width = safeAdd(contentSize.width, totalGap);
        }

        return contentSize;
    }

    void arrangeLinearPanel(
        StackPanelNode &panel,
        LinearArrangeContext &context)
    {
        if (!context.placeChild)
            return;

        const bool vertical = isVerticalOrientation(panel.getOrientation());
        const float gap = normalizedGap(panel.getGap());

        struct ChildPlacement
        {
            Node *node = nullptr;
            LayoutSize desired{};
        };

        std::vector<ChildPlacement> children;
        children.reserve(panel.getChildCount());

        float occupiedMain = 0.0f;

        for (std::size_t i = 0; i < panel.getChildCount(); ++i)
        {
            Node *child = panel.getChild(i);
            if (!child || !child->isVisible())
                continue;

            if (child->getPositionMode() == PositionMode::Absolute)
                continue;

            const LayoutSize desired =
                resolveFinalSize(*child, child->getDesiredSize());

            const float mainSize = vertical ? desired.height : desired.width;
            occupiedMain = safeAdd(occupiedMain, finiteOrZero(mainSize));
            children.push_back({child, desired});
        }

        if (children.size() > 1)
        {
            occupiedMain = safeAdd(
                occupiedMain,
                gap * static_cast<float>(children.size() - 1));
        }

        const float availableMain =
            vertical ? context.contentSize.height : context.contentSize.width;
        const float availableCross =
            vertical ? context.contentSize.width : context.contentSize.height;
        const float freeMain = std::max(
            0.0f,
            finiteOrZero(availableMain) - finiteOrZero(occupiedMain));

        float leading = 0.0f;
        float between = gap;

        switch (panel.getMainAlignment())
        {
        case MainAxisAlignment::CENTER:
            leading = freeMain * 0.5f;
            break;
        case MainAxisAlignment::END:
            leading = freeMain;
            break;
        case MainAxisAlignment::SPACE_BETWEEN:
            if (children.size() > 1)
                between = gap + freeMain / static_cast<float>(children.size() - 1);
            break;
        case MainAxisAlignment::START:
            break;
        }

        LayoutPosition position = context.contentPosition;
        if (vertical)
            position.y += leading;
        else
            position.x += leading;

        for (const ChildPlacement &placement : children)
        {
            LayoutSize finalSize = placement.desired;
            const float desiredCross = vertical ? finalSize.width : finalSize.height;
            const float crossFree = std::max(
                0.0f,
                availableCross - finiteOrZero(desiredCross));

            float crossOffset = 0.0f;
            switch (panel.getCrossAlignment())
            {
            case CrossAxisAlignment::CENTER:
                crossOffset = crossFree * 0.5f;
                break;
            case CrossAxisAlignment::END:
                crossOffset = crossFree;
                break;
            case CrossAxisAlignment::START:
                break;
            case CrossAxisAlignment::STRETCH:
                if (vertical)
                    finalSize.width = availableCross;
                else
                    finalSize.height = availableCross;
                break;
            }

            finalSize = resolveFinalSize(*placement.node, finalSize);

            const float finalCross = vertical ? finalSize.width : finalSize.height;
            const float finalCrossFree = std::max(
                0.0f,
                availableCross - finiteOrZero(finalCross));

            if (panel.getCrossAlignment() == CrossAxisAlignment::CENTER)
                crossOffset = finalCrossFree * 0.5f;
            else if (panel.getCrossAlignment() == CrossAxisAlignment::END)
                crossOffset = finalCrossFree;
            else
                crossOffset = 0.0f;

            LayoutPosition childPosition = position;
            if (vertical)
                childPosition.x += crossOffset;
            else
                childPosition.y += crossOffset;

            context.placeChild(*placement.node, childPosition, finalSize);

            const float mainSize = vertical ? finalSize.height : finalSize.width;
            if (vertical)
                position.y += finiteOrZero(mainSize) + between;
            else
                position.x += finiteOrZero(mainSize) + between;
        }
    }
}
