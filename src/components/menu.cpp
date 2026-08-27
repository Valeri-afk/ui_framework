#include "ui_framework/components/menu.hpp"
namespace ui
{
    Menu::Menu() : StackPanelNode(Orientation::Vertical) { setFocusable(false); setCapturable(false); setGap(0.0f); setMainAlignment(MainAxisAlignment::START); setCrossAlignment(CrossAxisAlignment::STRETCH); }
    MenuItem *Menu::addItem(std::unique_ptr<MenuItem> item, size_t index) { if(!item) return nullptr; MenuItem *raw=item.get(); raw->on<MenuItemActivatedEvent>([this](MenuItemActivatedEvent &, Node &node){ handleItemActivation(static_cast<MenuItem &>(node)); }); return static_cast<MenuItem*>(addChild(std::move(item), index)); }
    void Menu::removeItem(MenuItem &item) { if(&item==activeItem_) syncActiveItem(nullptr); if(&item==selectedItem_) syncSelectedItem(nullptr); removeChild(item); }
    void Menu::setActiveItem(MenuItem *item) noexcept { if(item==activeItem_) return; if(item && item->getParent()!=this) return; syncActiveItem(item); }
    MenuItem *Menu::getActiveItem() const noexcept { return activeItem_; }
    void Menu::setSelectedItem(MenuItem *item) noexcept { if(item==selectedItem_) return; if(item && item->getParent()!=this) return; syncSelectedItem(item); }
    MenuItem *Menu::getSelectedItem() const noexcept { return selectedItem_; }
    void Menu::setItemSpacing(float spacing) { setGap(spacing); }
    float Menu::getItemSpacing() const noexcept { return getGap(); }
    void Menu::syncActiveItem(MenuItem *item) noexcept { if(activeItem_) activeItem_->setHighlighted(false); activeItem_=item; if(activeItem_ && activeItem_->isEnabled()) activeItem_->setHighlighted(true); }
    void Menu::syncSelectedItem(MenuItem *item) noexcept { if(selectedItem_) selectedItem_->setSelected(false); selectedItem_=item; if(selectedItem_ && selectedItem_->isEnabled()) selectedItem_->setSelected(true); }
    void Menu::handleItemActivation(MenuItem &item) { if(!item.isEnabled()) return; setActiveItem(&item); setSelectedItem(&item); MenuItemActivatedEvent event; event.item=&item; emit(event); }
}
