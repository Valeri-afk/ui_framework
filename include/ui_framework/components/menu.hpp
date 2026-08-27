#pragma once

#include <cstddef>
#include <memory>

#include "ui_framework/components/menu_item.hpp"
#include "ui_framework/stack_panel_node.hpp"

namespace ui
{
    class Menu : public StackPanelNode
    {
    public:
        Menu();
        ~Menu() override = default;
        MenuItem *addItem(std::unique_ptr<MenuItem> item, size_t index = static_cast<size_t>(-1));
        void removeItem(MenuItem &item);
        void setActiveItem(MenuItem *item) noexcept;
        MenuItem *getActiveItem() const noexcept;
        void setSelectedItem(MenuItem *item) noexcept;
        MenuItem *getSelectedItem() const noexcept;
        void setItemSpacing(float spacing);
        float getItemSpacing() const noexcept;
    private:
        void syncActiveItem(MenuItem *item) noexcept;
        void syncSelectedItem(MenuItem *item) noexcept;
        void handleItemActivation(MenuItem &item);
        MenuItem *activeItem_ = nullptr;
        MenuItem *selectedItem_ = nullptr;
    };
}
