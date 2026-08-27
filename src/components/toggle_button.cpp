#include "ui_framework/components/toggle_button.hpp"
namespace ui
{
    ToggleButton::ToggleButton() = default;
    void ToggleButton::setSelected(bool selected) noexcept { selected_ = selected; }
    bool ToggleButton::isSelected() const noexcept { return selected_; }
    void ToggleButton::toggle()
    {
        if (!isEnabled() || !isVisible()) return;
        setSelected(!selected_);
        ToggleButtonToggledEvent event;
        event.selected = selected_;
        emit(event);
    }
    void ToggleButton::activate()
    {
        if (!isVisible() || !isEnabled()) return;
        toggle();
        Button::onActivate();
    }
    Color ToggleButton::presentationBackgroundColor() const noexcept
    {
        Color color = Button::presentationBackgroundColor();
        if (selected_){ color.r=static_cast<uint8_t>(color.r+(255-color.r)*0.16f); color.g=static_cast<uint8_t>(color.g+(255-color.g)*0.16f); color.b=static_cast<uint8_t>(color.b+(255-color.b)*0.16f); }
        return color;
    }
    Color ToggleButton::presentationBorderColor() const noexcept
    {
        Color color = Button::presentationBorderColor();
        if (selected_){ color.r=static_cast<uint8_t>(color.r+(255-color.r)*0.20f); color.g=static_cast<uint8_t>(color.g+(255-color.g)*0.20f); color.b=static_cast<uint8_t>(color.b+(255-color.b)*0.20f); }
        return color;
    }
    Color ToggleButton::presentationTextColor() const noexcept { return Button::presentationTextColor(); }
}
