#pragma once

#include <memory>
#include <string>

#include <SDL3_ttf/SDL_ttf.h>

#include "ui_framework/node.hpp"
#include "ui_framework/events.hpp"

namespace ui
{
    class TextContent;
    struct TabItemActivatedEvent : UIEvent {};

    class TabItem : public Node
    {
    public:
        TabItem();
        ~TabItem() override;
        TabItem(const TabItem &) = delete;
        TabItem &operator=(const TabItem &) = delete;
        void setText(std::string text);
        const std::string &getText() const noexcept;
        void setFont(TTF_Font *font);
        TTF_Font *getFont() const noexcept;
        void setTextColor(Color color) noexcept;
        Color getTextColor() const noexcept;
        void setBackgroundColor(Color color) noexcept;
        Color getBackgroundColor() const noexcept;
        void setActive(bool active) noexcept;
        bool isActive() const noexcept;
        void activate();
    protected:
        LayoutSize measureContent(const LayoutSize &availableContent) const override;
        void arrangeContent(const LayoutPosition &contentPosition, const LayoutSize &contentSize) override;
        void draw(SDL_Renderer *renderer) override;
    private:
        void handleMouseClick(MouseClickEvent &event);
        std::unique_ptr<TextContent> text_;
        Color textColor_ = Colors::white;
        Color backgroundColor_ = Colors::transparent;
        bool active_ = false;
    };
}