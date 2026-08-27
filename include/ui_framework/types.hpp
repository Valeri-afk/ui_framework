#pragma once

#include <algorithm>

#include "colors.hpp"

namespace ui
{
    enum class MainAxisAlignment
    {
        START,
        CENTER,
        END,
        SPACE_BETWEEN
    };
    enum class CrossAxisAlignment
    {
        START,
        CENTER,
        END,
        STRETCH
    };
    enum class TextAlignment
    {
        START,
        CENTER,
        END
    };
    enum class PositionMode
    {
        Layout,
        Absolute
    };
    enum class OutsideClickBehavior
    {
        Consume,
        Close
    };
    using BackdropClickBehavior = OutsideClickBehavior;

    struct ModalOptions
    {
        OutsideClickBehavior outsideClick = OutsideClickBehavior::Consume;
        bool closeOnEscape = true;
        bool showBackdrop = true;
    };

    struct LayoutPosition
    {
        float x = 0.0f;
        float y = 0.0f;
        bool operator==(const LayoutPosition &) const = default;
        LayoutPosition operator+(const LayoutPosition &rhs) const { return {x + rhs.x, y + rhs.y}; }
        LayoutPosition operator-(const LayoutPosition &rhs) const { return {x - rhs.x, y - rhs.y}; }
        LayoutPosition &operator+=(const LayoutPosition &rhs)
        {
            x += rhs.x;
            y += rhs.y;
            return *this;
        }
        LayoutPosition &operator-=(const LayoutPosition &rhs)
        {
            x -= rhs.x;
            y -= rhs.y;
            return *this;
        }
    };

    enum class LayoutValueType
    {
        Auto,
        Value
    };
    struct LayoutValue
    {
        LayoutValueType type = LayoutValueType::Auto;
        float value = 0.0f;
        static LayoutValue autoValue() noexcept { return {LayoutValueType::Auto, 0.0f}; }
        static LayoutValue fixed(float value) noexcept { return {LayoutValueType::Value, value}; }
        bool isAuto() const noexcept { return type == LayoutValueType::Auto; }
        bool isValue() const noexcept { return type == LayoutValueType::Value; }
    };
    struct LayoutSizeValue
    {
        LayoutValue width;
        LayoutValue height;
        static LayoutSizeValue autoSize() noexcept { return {LayoutValue::autoValue(), LayoutValue::autoValue()}; }
        static LayoutSizeValue fixed(float width, float height) noexcept { return {LayoutValue::fixed(width), LayoutValue::fixed(height)}; }
    };

    struct LayoutSize
    {
        float width = 0.0f;
        float height = 0.0f;
        bool operator==(const LayoutSize &rhs) const { return width == rhs.width && height == rhs.height; }
        bool operator!=(const LayoutSize &rhs) const { return !(*this == rhs); }
        bool operator<(const LayoutSize &rhs) const { return width != rhs.width ? width < rhs.width : height < rhs.height; }
        bool operator>(const LayoutSize &rhs) const { return rhs < *this; }
        bool operator<=(const LayoutSize &rhs) const { return !(*this > rhs); }
        bool operator>=(const LayoutSize &rhs) const { return !(*this < rhs); }
        LayoutSize operator+(const LayoutSize &rhs) const { return {width + rhs.width, height + rhs.height}; }
        LayoutSize operator-(const LayoutSize &rhs) const { return {width - rhs.width, height - rhs.height}; }
        LayoutSize operator*(float scalar) const { return {width * scalar, height * scalar}; }
        LayoutSize operator/(float scalar) const { return {width / scalar, height / scalar}; }
        LayoutSize &operator+=(const LayoutSize &rhs)
        {
            width += rhs.width;
            height += rhs.height;
            return *this;
        }
        LayoutSize &operator-=(const LayoutSize &rhs)
        {
            width -= rhs.width;
            height -= rhs.height;
            return *this;
        }
    };

    struct Padding
    {
        float left = 0.0f;
        float right = 0.0f;
        float top = 0.0f;
        float bottom = 0.0f;
        bool operator==(const Padding &) const = default;
    };
    struct Border
    {
        float left = 0.0f;
        float right = 0.0f;
        float top = 0.0f;
        float bottom = 0.0f;
        bool operator==(const Border &) const = default;
    };

    struct ScrollOffset
    {
        float x = 0.0f;
        float y = 0.0f;
        ScrollOffset operator+(const ScrollOffset &other) const noexcept { return {x + other.x, y + other.y}; }
        bool operator==(const ScrollOffset &) const = default;
    };

    struct StyleProps
    {
        Color backgroundColor = Colors::transparent;
        Color borderColor = Colors::transparent;
        float borderWidth = 0.0f;
        float borderRadius = 0.0f;
    };
}