#pragma once

#include "ui_framework/node.hpp"
#include "ui_framework/events.hpp"

namespace ui
{
    struct SliderValueChangedEvent : UIEvent
    {
        float value = 0.0f;
    };

    class Slider : public Node
    {
    public:
        Slider();
        ~Slider() override = default;

        void setMinimum(float minimum) noexcept;
        float getMinimum() const noexcept;
        void setMaximum(float maximum) noexcept;
        float getMaximum() const noexcept;
        void setValue(float value);
        float getValue() const noexcept;
        void setStep(float step) noexcept;
        float getStep() const noexcept;

    protected:
        LayoutSize measureContent(const LayoutSize &availableContent) const override;
        void draw(SDL_Renderer *renderer) override;

    private:
        void updateFromPointer(float x);
        void handleMouseDown(MouseDownEvent &event);
        void handleMouseMove(MouseMoveEvent &event);
        void handleMouseUp(MouseUpEvent &event);
        float minimum_ = 0.0f;
        float maximum_ = 1.0f;
        float value_ = 0.0f;
        float step_ = 0.0f;
        bool dragging_ = false;
    };
}
