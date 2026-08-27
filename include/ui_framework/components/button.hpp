#pragma once
#include <memory>
#include <string>
#include <SDL3_ttf/SDL_ttf.h>
#include "ui_framework/node.hpp"
#include "ui_framework/events.hpp"
namespace ui
{
class TextContent;
struct ButtonActivatedEvent : UIEvent {};
class Button : public Node
{
public:
 enum class Variant { FILLED, OUTLINED, TEXT };
 Button(); explicit Button(float borderRadius); ~Button() override;
 Button(const Button&)=delete; Button& operator=(const Button&)=delete;
 void setText(std::string text); const std::string& getText()const noexcept;
 void setFont(TTF_Font* font); TTF_Font* getFont()const noexcept;
 void setTextColor(Color color)noexcept; Color getTextColor()const noexcept;
 void setVariant(Variant variant)noexcept; Variant getVariant()const noexcept;
 void setBackgroundColor(Color color)noexcept; Color getBackgroundColor()const noexcept;
 void setBorderColor(Color color)noexcept; Color getBorderColor()const noexcept;
 void setBorderRadius(float radius)noexcept; float getBorderRadius()const noexcept;
 void setPressScale(float scale)noexcept; float getPressScale()const noexcept;
 void setPressAnimationEnabled(bool enabled)noexcept; bool isPressAnimationEnabled()const noexcept;
 bool isPressed()const noexcept; bool isHovered()const noexcept; virtual void activate();
 FloatAnimationProperty pressScaleProperty()noexcept{return makeFloatAnimationProperty(presentationScale_,&presentationScale_);}
protected:
 virtual void onActivate(); virtual Color presentationBackgroundColor()const noexcept; virtual Color presentationBorderColor()const noexcept; virtual Color presentationTextColor()const noexcept;
 LayoutSize measureContent(const LayoutSize& availableContent)const override; void arrangeContent(const LayoutPosition& contentPosition,const LayoutSize& contentSize)override; void draw(SDL_Renderer* renderer)override;
private:
 void setDefaultGeometry(); void handleMouseDown(MouseDownEvent& event); void handleMouseUp(MouseUpEvent& event); void handleMouseClick(MouseClickEvent& event); void handleMouseEnter(MouseEnterEvent& event); void handleMouseLeave(MouseLeaveEvent& event); void animatePressScale(float target)noexcept;
 static Color multiplyAlpha(Color color,float factor)noexcept; static Color lighten(Color color,float amount)noexcept;
 std::unique_ptr<TextContent> text_; Color textColor_=Colors::white; Variant variant_=Variant::FILLED; Color backgroundColor_=Colors::gray; Color borderColor_=Colors::black;
 float borderRadius_=4.0f; float pressScale_=0.96f; bool pressAnimationEnabled_=true; float presentationScale_=1.0f; bool pressed_=false; bool hovered_=false;
};
}
