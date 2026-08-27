#pragma once

#include "ui_framework/node.hpp"
#include "ui_framework/types.hpp"

namespace ui::internal
{
    LayoutSize resolveMeasurementProposal(
        const Node &node,
        LayoutSize proposal) noexcept;

    LayoutSize resolveFinalSize(
        const Node &node,
        LayoutSize allocated) noexcept;
}
