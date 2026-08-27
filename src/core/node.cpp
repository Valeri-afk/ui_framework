#include "ui_framework/node.hpp"

#include "layout_constraints.hpp"
#include "node_tree.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>

namespace
{
    float finiteOrZero(float value) noexcept { return std::isfinite(value) ? value : 0.0f; }
    float finiteOrInfinity(float value) noexcept { return std::isfinite(value) ? value : std::numeric_limits<float>::max(); }
    ui::LayoutSize sanitizeSize(ui::LayoutSize size) noexcept { return {finiteOrZero(size.width), finiteOrZero(size.height)}; }
    ui::LayoutSizeValue sanitizeSizeValue(ui::LayoutSizeValue size) noexcept
    {
        if (size.width.type == ui::LayoutValueType::Value)
            size.width.value = finiteOrZero(size.width.value);
        if (size.height.type == ui::LayoutValueType::Value)
            size.height.value = finiteOrZero(size.height.value);
        return size;
    }
    ui::LayoutPosition sanitizePosition(ui::LayoutPosition position) noexcept { return {finiteOrZero(position.x), finiteOrZero(position.y)}; }
    ui::Padding sanitizePadding(ui::Padding padding) noexcept
    {
        padding.left = finiteOrZero(padding.left);
        padding.right = finiteOrZero(padding.right);
        padding.top = finiteOrZero(padding.top);
        padding.bottom = finiteOrZero(padding.bottom);
        return padding;
    }
    ui::Border sanitizeBorder(ui::Border border) noexcept
    {
        border.left = finiteOrZero(border.left);
        border.right = finiteOrZero(border.right);
        border.top = finiteOrZero(border.top);
        border.bottom = finiteOrZero(border.bottom);
        return border;
    }
    void keepMaxAtLeastMin(ui::LayoutSize &minSize, ui::LayoutSize &maxSize) noexcept
    {
        maxSize.width = std::max(finiteOrInfinity(maxSize.width), minSize.width);
        maxSize.height = std::max(finiteOrInfinity(maxSize.height), minSize.height);
    }
    void keepMinAtMostMax(ui::LayoutSize &minSize, ui::LayoutSize &maxSize) noexcept
    {
        minSize.width = std::min(finiteOrInfinity(minSize.width), maxSize.width);
        minSize.height = std::min(finiteOrInfinity(minSize.height), maxSize.height);
    }
    void setMinWidthValue(ui::LayoutSize &minSize, ui::LayoutSize &maxSize, float width) noexcept
    {
        minSize.width = finiteOrZero(width);
        keepMaxAtLeastMin(minSize, maxSize);
    }
    void setMinHeightValue(ui::LayoutSize &minSize, ui::LayoutSize &maxSize, float height) noexcept
    {
        minSize.height = finiteOrZero(height);
        keepMaxAtLeastMin(minSize, maxSize);
    }
    void setMaxWidthValue(ui::LayoutSize &minSize, ui::LayoutSize &maxSize, float width) noexcept
    {
        maxSize.width = finiteOrZero(width);
        keepMinAtMostMax(minSize, maxSize);
    }
    void setMaxHeightValue(ui::LayoutSize &minSize, ui::LayoutSize &maxSize, float height) noexcept
    {
        maxSize.height = finiteOrZero(height);
        keepMinAtMostMax(minSize, maxSize);
    }
}
namespace ui
{
    void Node::invalidateLayout() noexcept
    {
        if (owner_)
            owner_->insertLayoutQueue(this);
    }

    void Node::animateFloat(
        const void *propertyKey,
        float currentValue,
        float targetValue,
        float duration,
        AnimationEasing easing,
        std::function<void(float)> setter)
    {
        if (owner_)
        {
            owner_->animationSystem_.animateFloat(
                *this,
                propertyKey,
                animationLifetimeToken_,
                currentValue,
                targetValue,
                duration,
                easing,
                std::move(setter));
        }
        else if (setter)
        {
            setter(targetValue);
        }
    }

    void Node::cancelAnimation(const void *propertyKey) noexcept
    {
        if (owner_)
            owner_->animationSystem_.cancel(*this, propertyKey);
    }

    void Node::dispatchEventImpl(UIEvent &event, const std::type_index &eventType, NodeTree &nodeTree)
    {
        (void)nodeTree;
        std::vector<std::function<void(UIEvent &, Node &)>> callbacks;
        callbacks.reserve(eventHandlers_.size());
        for (const EventHandlerRecord &record : eventHandlers_)
            if (record.eventType == eventType)
                callbacks.push_back(record.callback);
        for (auto &callback : callbacks)
            callback(event, *this);
    }

    Node::Node() = default;
    Node::~Node()
    {
        animationLifetimeToken_.reset();
    }
    Node::Id Node::getId() const noexcept { return id_; }
    Node *Node::getParent() const noexcept { return parent_; }
    void Node::setVisible(bool visible)
    {
        if (visible_ == visible)
            return;
        visible_ = visible;
    }
    bool Node::isVisible() const noexcept { return visible_; }
    void Node::setEnabled(bool enabled) noexcept { enabled_ = enabled; }
    bool Node::isEnabled() const noexcept { return enabled_; }
    void Node::setFocusable(bool focusable) noexcept { focusable_ = focusable; }
    bool Node::isFocusable() const noexcept { return focusable_; }
    void Node::setCapturable(bool capturable) noexcept { capturable_ = capturable; }
    bool Node::isCapturable() const noexcept { return capturable_; }
    void Node::setPosition(const LayoutPosition &position) { position_ = sanitizePosition(position); }
    LayoutPosition Node::getPosition() const noexcept { return position_; }
    void Node::setPositionMode(PositionMode positionMode) { positionMode_ = positionMode; }
    PositionMode Node::getPositionMode() const noexcept { return positionMode_; }
    void Node::setSize(const LayoutSizeValue &size) { size_ = sanitizeSizeValue(size); }
    LayoutSizeValue Node::getSize() const noexcept { return size_; }
    LayoutSize Node::getDesiredSize() const noexcept { return desiredSize_; }
    void Node::setMinSize(const LayoutSize &size)
    {
        minSize_ = sanitizeSize(size);
        keepMaxAtLeastMin(minSize_, maxSize_);
    }
    void Node::setMaxSize(const LayoutSize &size)
    {
        maxSize_ = sanitizeSize(size);
        keepMinAtMostMax(minSize_, maxSize_);
    }
    void Node::setMinWidth(float width) { setMinWidthValue(minSize_, maxSize_, finiteOrZero(width)); }
    void Node::setMinHeight(float height) { setMinHeightValue(minSize_, maxSize_, finiteOrZero(height)); }
    void Node::setMaxWidth(float width) { setMaxWidthValue(minSize_, maxSize_, finiteOrZero(width)); }
    void Node::setMaxHeight(float height) { setMaxHeightValue(minSize_, maxSize_, finiteOrZero(height)); }
    LayoutSize Node::getMinSize() const noexcept { return minSize_; }
    LayoutSize Node::getMaxSize() const noexcept { return maxSize_; }
    float Node::getMinWidth() const noexcept { return finiteOrZero(minSize_.width); }
    float Node::getMinHeight() const noexcept { return finiteOrZero(minSize_.height); }
    float Node::getMaxWidth() const noexcept { return finiteOrInfinity(maxSize_.width); }
    float Node::getMaxHeight() const noexcept { return finiteOrInfinity(maxSize_.height); }
    LayoutSize Node::clampSize(LayoutSize size, LayoutSize minSize, LayoutSize maxSize) const
    {
        size = sanitizeSize(size);
        minSize = sanitizeSize(minSize);
        maxSize = sanitizeSize(maxSize);
        maxSize.width = std::max(maxSize.width, minSize.width);
        maxSize.height = std::max(maxSize.height, minSize.height);
        return {std::clamp(size.width, minSize.width, maxSize.width), std::clamp(size.height, minSize.height, maxSize.height)};
    }
    void Node::setPadding(const Padding &padding) { padding_ = sanitizePadding(padding); }
    Padding Node::getPadding() const noexcept { return padding_; }
    void Node::setLeftPadding(float value) { padding_.left = finiteOrZero(value); }
    void Node::setRightPadding(float value) { padding_.right = finiteOrZero(value); }
    void Node::setTopPadding(float value) { padding_.top = finiteOrZero(value); }
    void Node::setBottomPadding(float value) { padding_.bottom = finiteOrZero(value); }
    void Node::setBorder(const Border &border) { border_ = sanitizeBorder(border); }
    Border Node::getBorder() const noexcept { return border_; }
    void Node::setLeftBorder(float value) { border_.left = finiteOrZero(value); }
    void Node::setRightBorder(float value) { border_.right = finiteOrZero(value); }
    void Node::setTopBorder(float value) { border_.top = finiteOrZero(value); }
    void Node::setBottomBorder(float value) { border_.bottom = finiteOrZero(value); }
    void Node::setClipToBounds(bool clip) noexcept { clipToBounds_ = clip; }
    bool Node::getClipToBounds() const noexcept { return clipToBounds_; }
    LayoutPosition Node::getActualPosition() const noexcept
    {
        const LayoutPosition position = actualPosition_;
        const CoordinateTransform &transform = coordinateTransform();
        return transform ? transform(*this, position) : position;
    }
    LayoutSize Node::getActualSize() const noexcept { return actualSize_; }
    Node *Node::getVisibleChild(size_t) const noexcept { return nullptr; }
    Node *Node::hitTest(float x, float y) noexcept
    {
        if (!isVisible() || !isEnabled())
            return nullptr;
        const float safeX = finiteOrZero(x), safeY = finiteOrZero(y);
        const LayoutPosition position = getActualPosition();
        const float posX = finiteOrZero(position.x), posY = finiteOrZero(position.y), width = finiteOrZero(actualSize_.width), height = finiteOrZero(actualSize_.height);
        if (safeX >= posX && safeY >= posY && safeX < posX + width && safeY < posY + height)
            return this;
        return nullptr;
    }
}
