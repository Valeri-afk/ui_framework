#pragma once

#include "node.hpp"
#include "panel_node.hpp"
#include "ui_framework/types.hpp"

namespace ui
{
    class StackPanelNode : public PanelNode
    {
    public:
        enum class Orientation
        {
            Vertical,
            Horizontal
        };

        explicit StackPanelNode(Orientation orientation = Orientation::Vertical);

        void setOrientation(Orientation orientation);
        Orientation getOrientation() const noexcept;

        void setGap(float gap);
        float getGap() const noexcept;

        void setMainAlignment(MainAxisAlignment alignment);
        MainAxisAlignment getMainAlignment() const noexcept;

        void setCrossAlignment(CrossAxisAlignment alignment);
        CrossAxisAlignment getCrossAlignment() const noexcept;

    protected:
        LayoutSize measure(const MeasureContext &context) const override;
        void arrange(const ArrangeContext &context) override;

    private:
        Orientation orientation_ = Orientation::Vertical;
        float gap_ = 0.0f;
        MainAxisAlignment mainAlignment_ = MainAxisAlignment::START;
        CrossAxisAlignment crossAlignment_ = CrossAxisAlignment::STRETCH;
    };
}
