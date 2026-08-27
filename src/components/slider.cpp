#include "ui_framework/components/slider.hpp"
#include "ui_framework/primitives.hpp"
#include "ui_framework/events.hpp"
#include <algorithm>
#include <cmath>
namespace ui
{
    Slider::Slider()
    {
        setFocusable(true); setCapturable(true); setSize(LayoutSizeValue::fixed(160.0f, 24.0f));
        on<MouseDownEvent>([this](MouseDownEvent &event, Node &) { handleMouseDown(event); });
        on<MouseMoveEvent>([this](MouseMoveEvent &event, Node &) { handleMouseMove(event); });
        on<MouseUpEvent>([this](MouseUpEvent &event, Node &) { handleMouseUp(event); });
    }
    void Slider::setMinimum(float minimum) noexcept { minimum_=minimum; if(maximum_<minimum_) maximum_=minimum_; setValue(value_); }
    float Slider::getMinimum() const noexcept { return minimum_; }
    void Slider::setMaximum(float maximum) noexcept { maximum_=std::max(minimum_,maximum); setValue(value_); }
    float Slider::getMaximum() const noexcept { return maximum_; }
    void Slider::setValue(float value)
    {
        const float old=value_; const float range=maximum_-minimum_; value=std::clamp(value,minimum_,maximum_);
        if(step_>0.0f&&range>0.0f){ value=minimum_+std::round((value-minimum_)/step_)*step_; value=std::clamp(value,minimum_,maximum_); }
        value_=value;
        if(value_!=old){ SliderValueChangedEvent event; event.value=value_; emit(event); }
    }
    float Slider::getValue() const noexcept { return value_; }
    void Slider::setStep(float step) noexcept { step_=std::max(0.0f,step); setValue(value_); }
    float Slider::getStep() const noexcept { return step_; }
    LayoutSize Slider::measureContent(const LayoutSize &) const { return {160.0f,24.0f}; }
    void Slider::updateFromPointer(float x){ const auto position=getActualPosition(); const auto size=getActualSize(); const float padding=std::min(8.0f,size.width*0.1f); const float left=position.x+padding; const float right=position.x+size.width-padding; const float usable=std::max(1.0f,right-left); const float t=std::clamp((x-left)/usable,0.0f,1.0f); setValue(minimum_+(maximum_-minimum_)*t); }
    void Slider::draw(SDL_Renderer *renderer){ if(!renderer)return; const auto position=getActualPosition(); const auto size=getActualSize(); const float padding=std::min(8.0f,size.width*0.1f); const float left=position.x+padding; const float right=position.x+size.width-padding; const float centerY=position.y+size.height*0.5f; const float trackHeight=std::max(2.0f,size.height*0.18f); primitives::roundedBoxRGBA(renderer,left,centerY-trackHeight*0.5f,right,centerY+trackHeight*0.5f,trackHeight*0.5f,110,110,110,255); const float range=maximum_-minimum_; const float t=range>0.0f?(value_-minimum_)/range:0.0f; const float thumbX=left+(right-left)*std::clamp(t,0.0f,1.0f); const float thumbRadius=std::max(5.0f,size.height*0.35f); primitives::filledCircleRGBA(renderer,thumbX,centerY,thumbRadius,230,230,230,255); }
    void Slider::handleMouseDown(MouseDownEvent &event){ if(event.button!=MouseButton::Left||!isEnabled())return; dragging_=true; updateFromPointer(event.position.x); }
    void Slider::handleMouseMove(MouseMoveEvent &event){ if(dragging_&&isEnabled())updateFromPointer(event.position.x); }
    void Slider::handleMouseUp(MouseUpEvent &event){ if(event.button==MouseButton::Left)dragging_=false; }
}
