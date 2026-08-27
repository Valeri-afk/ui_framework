#pragma once
#include <memory>
#include <SDL3/SDL.h>
#include "ui_framework/node.hpp"
#include "ui_framework/types.hpp"
namespace ui
{
class NodeTree; class InputSystem; class ModalSystem; class LayoutSystem; class ScrollSystem;
class UIManager
{
public:
 UIManager(); ~UIManager(); UIManager(const UIManager&)=delete; UIManager& operator=(const UIManager&)=delete;
 void processEvent(const SDL_Event& event,SDL_Renderer* renderer=nullptr); void advanceTime(float dt); void render(SDL_Renderer* renderer);
 Node* addRoot(std::unique_ptr<Node> node); Node* addOverlay(std::unique_ptr<Node> node); void removeRoot(Node* node); void removeOverlay(Node* node); void invalidateLayout(Node& node);
 bool enableScrolling(Node& node); bool disableScrolling(Node& node); bool isScrollingEnabled(const Node& node)const noexcept; bool setScrollOffset(Node& node,const ScrollOffset& offset); ScrollOffset getScrollOffset(const Node& node)const noexcept; ScrollOffset getMaximumScrollOffset(const Node& node)const noexcept;
 bool showModal(Node& node); bool showModal(Node& node,BackdropClickBehavior behavior); bool showModal(Node& node,const ModalOptions& options); bool closeModal(); bool isModal(const Node& node)const noexcept; Node* getActiveModal()const noexcept;
 void setBackdropColor(const Color& color)noexcept; Color getBackdropColor()const noexcept; void setBackdropFadeDuration(float seconds)noexcept; float getBackdropFadeDuration()const noexcept;
 AnimationController& animations() noexcept{return animationController_;} const AnimationController& animations()const noexcept{return animationController_;}
private:
 void draw(SDL_Renderer* renderer); void prepareForTreeOperation(); void syncState();
 std::unique_ptr<NodeTree> nodeTree_; std::unique_ptr<InputSystem> inputSystem_; std::unique_ptr<ModalSystem> modalSystem_; std::unique_ptr<LayoutSystem> layoutSystem_; std::unique_ptr<ScrollSystem> scrollSystem_; AnimationController animationController_;
};
}
