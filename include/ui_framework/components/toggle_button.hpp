#pragma once

#include "ui_framework/components/button.hpp"

namespace ui
{
    struct ToggleButtonToggledEvent : UIEvent
    {
        bool selected = false;
    };

    class ToggleButton : public Button
    {
    public:
        ToggleButton();
        ~ToggleButton() override = default;
        void setSelected(bool selected) noexcept;
        bool isSelected() const noexcept;
        void toggle();
        void activate() override;
    protected:
        Color presentationBackgroundColor() const noexcept override;
        Color presentationBorderColor() const noexcept override;
        Color presentationTextColor() const noexcept override;
    private:
        bool selected_ = false;
    };
}
