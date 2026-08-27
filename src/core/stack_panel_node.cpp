#include "ui_framework/stack_panel_node.hpp"
#include "linear_layout.hpp"

#include <cmath>

namespace ui
{
    StackPanelNode::StackPanelNode(Orientation orientation)
        : orientation_(orientation)
    {
    }

    void StackPanelNode::setOrientation(Orientation orientation)
    {
        if (orientation_ == orientation)
            return;
        orientation_ = orientation;
    }

    StackPanelNode::Orientation StackPanelNode::getOrientation() const noexcept
    {
        return orientation_;
    }

    void StackPanelNode::setGap(float gap)
    {
        if (!std::isfinite(gap) || gap < 0.0f || gap_ == gap)
            return;
        gap_ = gap;
    }

    float StackPanelNode::getGap() const noexcept
    {
        return gap_;
    }

    void StackPanelNode::setMainAlignment(MainAxisAlignment alignment)
    {
        if (mainAlignment_ == alignment)
            return;
        mainAlignment_ = alignment;
    }

    MainAxisAlignment StackPanelNode::getMainAlignment() const noexcept
    {
        return mainAlignment_;
    }

    void StackPanelNode::setCrossAlignment(CrossAxisAlignment alignment)
    {
        if (crossAlignment_ == alignment)
            return;
        crossAlignment_ = alignment;
    }

    CrossAxisAlignment StackPanelNode::getCrossAlignment() const noexcept
    {
        return crossAlignment_;
    }

    LayoutSize StackPanelNode::measure(const MeasureContext &context) const
    {
        internal::LinearMeasureContext linearContext;
        linearContext.availableSize = context.availableContentSize;
        linearContext.measureChild = context.measureChild;
        return internal::measureLinearPanel(*this, linearContext);
    }

    void StackPanelNode::arrange(const ArrangeContext &context)
    {
        internal::LinearArrangeContext linearContext;
        linearContext.contentPosition = context.contentPosition;
        linearContext.contentSize = context.contentSize;
        linearContext.placeChild = context.arrangeChild;
        internal::arrangeLinearPanel(*this, linearContext);
    }
}
