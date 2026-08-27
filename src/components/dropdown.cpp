#include "ui_framework/components/dropdown.hpp"
#include <limits>
#include <utility>
namespace ui
{
    namespace { constexpr size_t INVALID_INDEX=std::numeric_limits<size_t>::max(); }
    Dropdown::Dropdown() {
        auto trigger=std::make_unique<Button>(); trigger_=trigger.get(); trigger_->setText(placeholder_); trigger_->on<ButtonActivatedEvent>([this](ButtonActivatedEvent &, Node &){handleTriggerActivate();});
        auto menu=std::make_unique<Menu>(); menu_=menu.get(); menu_->setVisible(false); menu_->setPositionMode(PositionMode::Absolute); menu_->setMainAlignment(MainAxisAlignment::START); menu_->setCrossAlignment(CrossAxisAlignment::STRETCH); menu_->on<MenuItemActivatedEvent>([this](MenuItemActivatedEvent &event, Node &){if(event.item) handleItemActivate(*event.item);});
        addChild(std::move(trigger),0); addChild(std::move(menu),1);
    }
    MenuItem *Dropdown::addItem(std::unique_ptr<MenuItem> item,size_t index){ if(!item||!menu_) return nullptr; return menu_->addItem(std::move(item),index); }
    void Dropdown::removeItem(MenuItem &item){ if(menu_) menu_->removeItem(item); if(selectedItem_==&item){selectedItem_=nullptr;selectedIndex_=INVALID_INDEX;if(trigger_)trigger_->setText(placeholder_);invalidateLayout();} }
    void Dropdown::open(){if(!menu_||!isEnabled())return;syncMenuGeometry();menu_->setVisible(true);invalidateLayout();}
    void Dropdown::close() noexcept{if(!menu_||!menu_->isVisible())return;menu_->setVisible(false);invalidateLayout();}
    void Dropdown::toggle(){if(isOpen())close();else open();}
    bool Dropdown::isOpen() const noexcept{return menu_&&menu_->isVisible();}
    MenuItem *Dropdown::getSelectedItem() const noexcept{return selectedItem_;}
    size_t Dropdown::getSelectedIndex() const noexcept{return selectedIndex_;}
    void Dropdown::clearSelection() noexcept{if(selectedItem_)selectedItem_->setSelected(false);selectedItem_=nullptr;selectedIndex_=INVALID_INDEX;if(trigger_)trigger_->setText(placeholder_);invalidateLayout();}
    Button &Dropdown::getTrigger() noexcept{return *trigger_;}
    const Button &Dropdown::getTrigger() const noexcept{return *trigger_;}
    Menu &Dropdown::getMenu() noexcept{return *menu_;}
    const Menu &Dropdown::getMenu() const noexcept{return *menu_;}
    void Dropdown::setPlaceholder(std::string text){placeholder_=std::move(text);if(!selectedItem_&&trigger_)trigger_->setText(placeholder_);}
    const std::string &Dropdown::getPlaceholder() const noexcept{return placeholder_;}
    void Dropdown::handleTriggerActivate(){toggle();}
    void Dropdown::handleItemActivate(MenuItem &item){size_t index=INVALID_INDEX;if(menu_){for(size_t i=0;i<menu_->getChildCount();++i){if(menu_->getChild(i)==&item){index=i;break;}}}if(index==INVALID_INDEX||!item.isEnabled()||!item.isVisible())return;if(selectedItem_&&selectedItem_!=&item)selectedItem_->setSelected(false);selectedItem_=&item;selectedIndex_=index;selectedItem_->setSelected(true);if(trigger_)trigger_->setText(item.getText());close();DropdownSelectionChangedEvent event;event.selectedItem=selectedItem_;emit(event);}
    void Dropdown::syncMenuGeometry(){if(!trigger_||!menu_)return;const auto p=trigger_->getPosition();const auto s=trigger_->getDesiredSize();menu_->setPosition({p.x,p.y+s.height});menu_->setSize(LayoutSizeValue{{LayoutValueType::Value,s.width},LayoutValue::autoValue()});}
}
