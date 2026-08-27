#pragma once

#include <memory>
#include <string>

#include <SDL3_ttf/SDL_ttf.h>

#include "ui_framework/node.hpp"
#include "ui_framework/events.hpp"

namespace ui
{
    class TextContent;
    class MenuItem;
    struct MenuItemActivatedEvent : UIEvent { MenuItem *item = nullptr; };

    class MenuItem : public Node
    {
    public:
        MenuItem();
        ~MenuItem() override;
        MenuItem(const MenuItem &) = delete;
        MenuItem &operator=(const MenuItem &) = delete;
        void setText(std::string text);
        const std::string &getText() const noexcept;
        void setFont(TTF_Font *font);
        TTF_Font *getFont() const noexcept;
        void setTextColor(Color color) noexcept;
        Color getTextColor() const noexcept;
        void setBackgroundColor(Color color) noexcept;
        Color getBackgroundColor() const noexcept;
        void setHighlighted(bool highlighted) noexcept;
        bool isHighlighted() const noexcept;
        void setSelected(bool selected) noexcept;
        bool isSelected() const noexcept;
        void activate();
    protected:
        LayoutSize measureContent(const LayoutSize &availableContent) const override;
        void arrangeContent(const LayoutPosition &contentPosition, const LayoutSize &contentSize) override;
        void draw(SDL_Renderer *renderer) override;
    private:
        void handleMouseEnter(MouseEnterEvent &event);
        void handleMouseLeave(MouseLeaveEvent &event);
        void handleMouseClick(MouseClickEvent &event);
        std::unique_ptr<TextContent> text_;
        Color textColor_ = Colors::white;
        Color backgroundColor_ = Colors::transparent;
        bool highlighted_ = false;
        bool selected_ = false;
    };
}
