#pragma once

#include <cstddef>
#include <memory>
#include <string>

#include "ui_framework/components/button.hpp"
#include "ui_framework/components/menu.hpp"
#include "ui_framework/panel_node.hpp"

namespace ui
{
    struct DropdownSelectionChangedEvent : UIEvent
    {
        MenuItem *selectedItem = nullptr;
    };

    class Dropdown : public PanelNode
    {
    public:
        Dropdown();
        ~Dropdown() override = default;
        MenuItem *addItem(std::unique_ptr<MenuItem> item, size_t index = static_cast<size_t>(-1));
        void removeItem(MenuItem &item);
        void open();
        void close() noexcept;
        void toggle();
        bool isOpen() const noexcept;
        MenuItem *getSelectedItem() const noexcept;
        size_t getSelectedIndex() const noexcept;
        void clearSelection() noexcept;
        Button &getTrigger() noexcept;
        const Button &getTrigger() const noexcept;
        Menu &getMenu() noexcept;
        const Menu &getMenu() const noexcept;
        void setPlaceholder(std::string text);
        const std::string &getPlaceholder() const noexcept;
    private:
        void handleTriggerActivate();
        void handleItemActivate(MenuItem &item);
        void syncMenuGeometry();
        Button *trigger_ = nullptr;
        Menu *menu_ = nullptr;
        MenuItem *selectedItem_ = nullptr;
        size_t selectedIndex_ = static_cast<size_t>(-1);
        std::string placeholder_ = "Select...";
    };
}
