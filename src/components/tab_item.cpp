#include "ui_framework/components/tab_item.hpp"
#include "ui_framework/primitives.hpp"
#include "../core/text_content.hpp"
#include <utility>
namespace ui
{
    TabItem::TabItem() : text_(std::make_unique<TextContent>()) { setPadding({12.0f,12.0f,8.0f,8.0f}); setFocusable(true); setCapturable(true); on<MouseClickEvent>([this](MouseClickEvent &event, Node &) { handleMouseClick(event); }); }
    TabItem::~TabItem() = default;
    void TabItem::setText(std::string text) { if(text_->getText()==text) return; text_->setText(std::move(text)); }
    const std::string &TabItem::getText() const noexcept { return text_->getText(); }
    void TabItem::setFont(TTF_Font *font) { if(text_->getFont()==font) return; text_->setFont(font); }
    TTF_Font *TabItem::getFont() const noexcept { return text_->getFont(); }
    void TabItem::setTextColor(Color color) noexcept { textColor_=color; text_->setColor(color); }
    Color TabItem::getTextColor() const noexcept { return textColor_; }
    void TabItem::setBackgroundColor(Color color) noexcept { backgroundColor_=color; }
    Color TabItem::getBackgroundColor() const noexcept { return backgroundColor_; }
    void TabItem::setActive(bool active) noexcept { active_=active; }
    bool TabItem::isActive() const noexcept { return active_; }
    void TabItem::activate() { if(!isVisible() || !isEnabled()) return; TabItemActivatedEvent event; emit(event); }
    LayoutSize TabItem::measureContent(const LayoutSize &availableContent) const { return text_->measure(availableContent.width); }
    void TabItem::arrangeContent(const LayoutPosition &contentPosition, const LayoutSize &contentSize) { text_->setHorizontalAlignment(TextAlignment::CENTER); text_->setVerticalAlignment(TextAlignment::CENTER); text_->arrange(contentPosition, contentSize); }
    void TabItem::draw(SDL_Renderer *renderer) { if(!renderer) return; const auto position=getActualPosition(); const auto size=getActualSize(); Color background=backgroundColor_; if(active_){ background.r=static_cast<uint8_t>(background.r+(255-background.r)*0.16f); background.g=static_cast<uint8_t>(background.g+(255-background.g)*0.16f); background.b=static_cast<uint8_t>(background.b+(255-background.b)*0.16f); } if(background.a>0) primitives::boxRGBA(renderer,position.x,position.y,position.x+size.width,position.y+size.height,background.r,background.g,background.b,background.a); text_->draw(renderer); }
    void TabItem::handleMouseClick(MouseClickEvent &event) { if(event.button==MouseButton::Left) activate(); }
}
