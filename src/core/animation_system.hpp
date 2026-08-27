#pragma once
#include <cstdint>
#include <functional>
#include <memory>
#include <vector>
#include "ui_framework/animation.hpp"
namespace ui
{
class Node;
class AnimationSystem
{
public:
 using PropertyKey=const void*; using AnimationId=std::uint64_t; using Setter=std::function<void(float)>;
 AnimationSystem()=default; AnimationSystem(const AnimationSystem&)=delete; AnimationSystem& operator=(const AnimationSystem&)=delete;
 void animateFloat(Node& owner,PropertyKey property,std::weak_ptr<void> lifetime,float currentValue,float targetValue,float duration,AnimationEasing easing,Setter setter);
 void cancel(Node& owner,PropertyKey property) noexcept; void advance(float dt) noexcept;
private:
 struct ActiveAnimation{AnimationId id=0;Node* owner=nullptr;PropertyKey property=nullptr;std::weak_ptr<void> lifetime;float startValue=0.0f;float currentValue=0.0f;float targetValue=0.0f;float duration=0.0f;float elapsed=0.0f;AnimationEasing easing=AnimationEasing::Linear;Setter setter;};
 std::vector<ActiveAnimation> animations_; AnimationId nextAnimationId_=1;
 static float applyEasing(float t,AnimationEasing easing) noexcept; static bool nearlyEqual(float a,float b) noexcept; void removeFor(Node& owner,PropertyKey property) noexcept;
};
}
