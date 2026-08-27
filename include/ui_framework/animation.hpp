#pragma once
#include <functional>
#include <memory>
namespace ui
{
class Node;
enum class AnimationEasing { Linear, EaseIn, EaseOut, EaseInOut };
class FloatAnimationProperty
{
public:
 using PropertyKey=const void*; using Getter=std::function<float()>; using Setter=std::function<void(float)>; using Animate=std::function<void(float,float,float,AnimationEasing)>; using Cancel=std::function<void()>;
 FloatAnimationProperty()=default;
 explicit operator bool()const noexcept{return key_!=nullptr&&static_cast<bool>(getter_)&&static_cast<bool>(setter_)&&(!owner_||!lifetime_.expired());}
 bool isValid()const noexcept{return static_cast<bool>(*this);}
 float value()const{return isValid()?getter_():0.0f;}
 void set(float value)const{if(isValid())setter_(value);}
private:
 friend class Node; friend class AnimationController;
 FloatAnimationProperty(Node* owner,PropertyKey key,Getter getter,Setter setter,Animate animate,Cancel cancel,std::weak_ptr<void> lifetime):owner_(owner),key_(key),getter_(std::move(getter)),setter_(std::move(setter)),animate_(std::move(animate)),cancel_(std::move(cancel)),lifetime_(std::move(lifetime)){}
 Node* owner_=nullptr; PropertyKey key_=nullptr; Getter getter_; Setter setter_; Animate animate_; Cancel cancel_; std::weak_ptr<void> lifetime_;
};
class AnimationController
{
public:
 bool to(const FloatAnimationProperty& property,float targetValue,float duration,AnimationEasing easing=AnimationEasing::Linear)const{if(!property||!property.animate_)return false;property.animate_(property.value(),targetValue,duration,easing);return true;}
 bool cancel(const FloatAnimationProperty& property)const noexcept{if(!property||!property.cancel_)return false;property.cancel_();return true;}
};
}
