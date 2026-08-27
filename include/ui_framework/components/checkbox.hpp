#pragma once

#include "ui_framework/node.hpp"
#include "ui_framework/events.hpp"

namespace ui
{
    struct CheckboxToggledEvent : UIEvent
    {
        bool checked = false;
    };

    class Checkbox : public Node
    {
    public:
        Checkbox();
        ~Checkbox() override = default;

        void setChecked(bool checked) noexcept;
        bool isChecked() const noexcept;
        void setBoxSize(float size) noexcept;
        float getBoxSize() const noexcept;
        void toggle();
        void activate();

    protected:
        LayoutSize measureContent(const LayoutSize &availableContent) const override;
        void draw(SDL_Renderer *renderer) override;

    private:
        bool checked_ = false;
        float boxSize_ = 20.0f;
    };
}
