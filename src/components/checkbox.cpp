#include "ui_framework/components/checkbox.hpp"

#include "ui_framework/primitives.hpp"
#include "ui_framework/events.hpp"

#include <algorithm>

namespace ui
{
    Checkbox::Checkbox()
    {
        setFocusable(true);
        setCapturable(true);
        setSize(LayoutSizeValue::fixed(boxSize_, boxSize_));
        on<MouseClickEvent>([this](MouseClickEvent &event, Node &)
        {
            if (event.button == MouseButton::Left)
                activate();
        });
    }

    void Checkbox::setChecked(bool checked) noexcept { checked_ = checked; }
    bool Checkbox::isChecked() const noexcept { return checked_; }

    void Checkbox::setBoxSize(float size) noexcept
    {
        boxSize_ = std::max(1.0f, size);
        setSize(LayoutSizeValue::fixed(boxSize_, boxSize_));
    }

    float Checkbox::getBoxSize() const noexcept { return boxSize_; }

    void Checkbox::toggle()
    {
        if (!isVisible() || !isEnabled())
            return;
        checked_ = !checked_;
        CheckboxToggledEvent event;
        event.checked = checked_;
        emit(event);
    }

    void Checkbox::activate() { toggle(); }

    LayoutSize Checkbox::measureContent(const LayoutSize &) const
    {
        return {boxSize_, boxSize_};
    }

    void Checkbox::draw(SDL_Renderer *renderer)
    {
        if (!renderer)
            return;

        const auto p = getActualPosition();
        const auto s = getActualSize();
        const Uint8 border = 220;
        primitives::roundedRectangleRGBA(renderer, p.x, p.y,
                                         p.x + s.width, p.y + s.height, 3.0f,
                                         border, border, border, 255);

        if (!checked_)
            return;

        const float inset = std::max(2.0f, s.width * 0.22f);
        const float x1 = p.x + inset;
        const float y1 = p.y + s.height * 0.52f;
        const float x2 = p.x + s.width * 0.43f;
        const float y2 = p.y + s.height - inset;
        const float x3 = p.x + s.width - inset;
        const float y3 = p.y + inset;
        primitives::lineRGBA(renderer, x1, y1, x2, y2, 255, 255, 255, 255);
        primitives::lineRGBA(renderer, x2, y2, x3, y3, 255, 255, 255, 255);
    }
}
