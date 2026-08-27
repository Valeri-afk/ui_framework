#pragma once
#include <SDL3/SDL.h>
#include "node_tree.hpp"
#include "ui_framework/types.hpp"
namespace ui
{
    class LayoutSystem
    {
    public:
        LayoutSystem();
        LayoutSystem(const LayoutSystem &) = delete;
        LayoutSystem &operator=(const LayoutSystem &) = delete;
        void setViewportSize(const LayoutSize &size) noexcept;
        LayoutSize getViewportSize() const noexcept;
        bool syncViewportFromRenderer(SDL_Renderer *renderer);
        void requestFullLayout(NodeTree &nodeTree);
        void processLayoutQueue(NodeTree &nodeTree);

    private:
        void measureRecursive(Node &node, const LayoutSize &availableBorderBoxSize, NodeTree &nodeTree);
        void arrangeRecursive(Node &node, NodeTree &nodeTree);
        LayoutSize makeRootAvailableSize(const Node &root) const;
        LayoutSize viewportSize_{};
    };
}
