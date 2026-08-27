#include "ui_framework/components/tab_control.hpp"

#include <limits>

namespace ui
{
    namespace { constexpr size_t INVALID_INDEX = std::numeric_limits<size_t>::max(); }

    TabControl::TabControl() : StackPanelNode(Orientation::Horizontal) { setGap(0.0f); }

    TabItem *TabControl::addTab(std::unique_ptr<TabItem> tab, size_t index)
    {
        if (!tab) return nullptr;
        TabItem *raw = tab.get();
        raw->on<TabItemActivatedEvent>([this](TabItemActivatedEvent &, Node &node) { handleTabActivation(static_cast<TabItem &>(node)); });
        Node *attached = addChild(std::move(tab), index);
        if (attached != raw) return nullptr;
        if (!selectedTab_) selectTab(*raw);
        return raw;
    }

    void TabControl::removeTab(TabItem &tab)
    {
        const bool wasSelected = selectedTab_ == &tab;
        removeChild(tab);
        if (!wasSelected) return;
        selectedTab_ = nullptr;
        selectedIndex_ = INVALID_INDEX;
        for (size_t i = 0; i < getChildCount(); ++i)
            if (asTabItem(getChild(i))) { selectTab(i); break; }
    }

    bool TabControl::selectTab(size_t index)
    {
        if (index >= getChildCount()) return false;
        auto *tab = asTabItem(getChild(index));
        if (!tab || !tab->isEnabled() || !tab->isVisible()) return false;
        return selectTab(*tab);
    }

    bool TabControl::selectTab(TabItem &tab)
    {
        size_t index = INVALID_INDEX;
        for (size_t i = 0; i < getChildCount(); ++i)
            if (getChild(i) == &tab) { index = i; break; }
        if (index == INVALID_INDEX || !tab.isEnabled() || !tab.isVisible()) return false;
        if (selectedTab_ == &tab) { tab.setActive(true); selectedIndex_ = index; return true; }
        if (selectedTab_) selectedTab_->setActive(false);
        selectedTab_ = &tab;
        selectedIndex_ = index;
        selectedTab_->setActive(true);
        TabSelectionChangedEvent event;
        event.selectedTab = selectedTab_;
        event.selectedIndex = selectedIndex_;
        emit(event);
        return true;
    }

    void TabControl::clearSelection()
    {
        if (selectedTab_) selectedTab_->setActive(false);
        selectedTab_ = nullptr;
        selectedIndex_ = INVALID_INDEX;
    }

    TabItem *TabControl::getSelectedTab() const noexcept { return selectedTab_; }
    size_t TabControl::getSelectedIndex() const noexcept { return selectedIndex_; }
    void TabControl::handleTabActivation(TabItem &tab) { selectTab(tab); }
    TabItem *TabControl::asTabItem(Node *node) const noexcept { return dynamic_cast<TabItem *>(node); }
}
