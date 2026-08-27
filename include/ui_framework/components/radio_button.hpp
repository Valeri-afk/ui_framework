#pragma once

#include "ui_framework/node.hpp"
#include "ui_framework/events.hpp"

namespace ui
{
    struct RadioButtonSelectedEvent : UIEvent {};

    class RadioButton : public Node
    {
    public:
        RadioButton();
        ~RadioButton() override = default;

        void setSelected(bool selected) noexcept;
        bool isSelected() const noexcept;
        void setRadius(float radius) noexcept;
        float getRadius() const noexcept;
        void select();
        void activate();

    protected:
        LayoutSize measureContent(const LayoutSize &availableContent) const override;
        void draw(SDL_Renderer *renderer) override;

    private:
        bool selected_ = false;
        float radius_ = 10.0f;
    };
}
