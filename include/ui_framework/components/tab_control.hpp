#pragma once

#include <cstddef>
#include <memory>

#include "ui_framework/components/tab_item.hpp"
#include "ui_framework/stack_panel_node.hpp"

namespace ui
{
    struct TabSelectionChangedEvent : UIEvent
    {
        TabItem *selectedTab = nullptr;
        size_t selectedIndex = static_cast<size_t>(-1);
    };

    class TabControl : public StackPanelNode
    {
    public:
        TabControl();
        ~TabControl() override = default;
        TabItem *addTab(std::unique_ptr<TabItem> tab, size_t index = static_cast<size_t>(-1));
        void removeTab(TabItem &tab);
        bool selectTab(size_t index);
        bool selectTab(TabItem &tab);
        void clearSelection();
        TabItem *getSelectedTab() const noexcept;
        size_t getSelectedIndex() const noexcept;
    private:
        void handleTabActivation(TabItem &tab);
        TabItem *asTabItem(Node *node) const noexcept;
        TabItem *selectedTab_ = nullptr;
        size_t selectedIndex_ = static_cast<size_t>(-1);
    };
}
