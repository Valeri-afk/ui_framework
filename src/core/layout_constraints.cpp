#include "layout_constraints.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace
{
    constexpr float kInfinity = std::numeric_limits<float>::max();
    float finiteOrZero(float value) noexcept { return std::isfinite(value) ? value : 0.0f; }
    float finiteOrInfinity(float value) noexcept { return std::isfinite(value) ? value : kInfinity; }
    float proposalBoundedByMax(float proposal, float maxValue) noexcept { return std::min(finiteOrInfinity(proposal), finiteOrInfinity(maxValue)); }
    float resolveFinalDimension(float allocated, float explicitValue, const ui::LayoutValue &sizeValue, float minValue, float maxValue) noexcept
    {
        const float minResolved = finiteOrZero(minValue);
        const float maxResolved = std::max(minResolved, finiteOrInfinity(maxValue));
        const float requested = sizeValue.isValue() ? finiteOrZero(explicitValue) : finiteOrZero(allocated);
        return std::clamp(requested, minResolved, maxResolved);
    }
}

namespace ui::internal
{
    LayoutSize resolveMeasurementProposal(const Node &node, LayoutSize proposal) noexcept
    {
        proposal.width = finiteOrInfinity(proposal.width);
        proposal.height = finiteOrInfinity(proposal.height);
        const LayoutSizeValue size = node.getSize();
        const LayoutSize maxSize = node.getMaxSize();
        proposal.width = size.width.isValue() ? proposalBoundedByMax(size.width.value, maxSize.width) : proposalBoundedByMax(proposal.width, maxSize.width);
        proposal.height = size.height.isValue() ? proposalBoundedByMax(size.height.value, maxSize.height) : proposalBoundedByMax(proposal.height, maxSize.height);
        return proposal;
    }
    LayoutSize resolveFinalSize(const Node &node, LayoutSize allocated) noexcept
    {
        allocated.width = finiteOrZero(allocated.width);
        allocated.height = finiteOrZero(allocated.height);
        const LayoutSizeValue size = node.getSize();
        const LayoutSize minSize = node.getMinSize();
        const LayoutSize maxSize = node.getMaxSize();
        allocated.width = resolveFinalDimension(allocated.width, size.width.value, size.width, minSize.width, maxSize.width);
        allocated.height = resolveFinalDimension(allocated.height, size.height.value, size.height, minSize.height, maxSize.height);
        return allocated;
    }
}