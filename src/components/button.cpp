#include "ui_framework/components/button.hpp"
#include "ui_framework/primitives.hpp"
#include "ui_framework/events.hpp"
#include "../core/text_content.hpp"
#include <algorithm>
#include <cmath>
#include <utility>
namespace ui
{
    Button::Button() : text_(std::make_unique<TextContent>())
    {
        setDefaultGeometry(); setFocusable(true); setCapturable(true);
        on<MouseDownEvent>([this](MouseDownEvent &e, Node &) { handleMouseDown(e); });
        on<MouseUpEvent>([this](MouseUpEvent &e, Node &) { handleMouseUp(e); });
        on<MouseClickEvent>([this](MouseClickEvent &e, Node &) { handleMouseClick(e); });
        on<MouseEnterEvent>([this](MouseEnterEvent &e, Node &) { handleMouseEnter(e); });
        on<MouseLeaveEvent>([this](MouseLeaveEvent &e, Node &) { handleMouseLeave(e); });
    }
    Button::Button(float borderRadius) : Button() { setBorderRadius(borderRadius); }
    Button::~Button() = default;
    void Button::setDefaultGeometry() { setPadding({12.0f,12.0f,8.0f,8.0f}); setBorder({1.0f,1.0f,1.0f,1.0f}); }
    void Button::setText(std::string text) { if(text_->getText()==text)return; text_->setText(std::move(text)); }
    const std::string &Button::getText() const noexcept { return text_->getText(); }
    void Button::setFont(TTF_Font *font) { if(text_->getFont()==font)return; text_->setFont(font); }
    TTF_Font *Button::getFont() const noexcept { return text_->getFont(); }
    void Button::setTextColor(Color c) noexcept { textColor_=c; text_->setColor(c); }
    Color Button::getTextColor() const noexcept { return textColor_; }
    void Button::setVariant(Variant v) noexcept { variant_=v; }
    Button::Variant Button::getVariant() const noexcept { return variant_; }
    void Button::setBackgroundColor(Color c) noexcept { backgroundColor_=c; }
    Color Button::getBackgroundColor() const noexcept { return backgroundColor_; }
    void Button::setBorderColor(Color c) noexcept { borderColor_=c; }
    Color Button::getBorderColor() const noexcept { return borderColor_; }
    void Button::setBorderRadius(float r) noexcept { borderRadius_=std::max(0.0f,r); }
    float Button::getBorderRadius() const noexcept { return borderRadius_; }
    void Button::setPressScale(float s) noexcept { pressScale_=std::clamp(s,0.0f,1.0f); }
    float Button::getPressScale() const noexcept { return pressScale_; }
    void Button::setPressAnimationEnabled(bool e) noexcept
    {
        pressAnimationEnabled_=e;
        if(!e)
        {
            cancelProperty(pressScaleProperty());
            presentationScale_=1.0f;
        }
    }
    bool Button::isPressAnimationEnabled() const noexcept { return pressAnimationEnabled_; }
    bool Button::isPressed() const noexcept { return pressed_; }
    bool Button::isHovered() const noexcept { return hovered_; }
    void Button::activate() { if(!isVisible()||!isEnabled())return; onActivate(); }
    void Button::onActivate() { ButtonActivatedEvent event; emit(event); }
    Color Button::presentationBackgroundColor() const noexcept { return backgroundColor_; }
    Color Button::presentationBorderColor() const noexcept { return borderColor_; }
    Color Button::presentationTextColor() const noexcept { return textColor_; }
    LayoutSize Button::measureContent(const LayoutSize &available) const { return text_->measure(available.width); }
    void Button::arrangeContent(const LayoutPosition &contentPosition,const LayoutSize &contentSize){text_->arrange(contentPosition,contentSize);}
    void Button::draw(SDL_Renderer *renderer)
    {
        if(!renderer)return;
        const auto p=getActualPosition(); const auto s=getActualSize(); const auto bg=presentationBackgroundColor(); const auto border=presentationBorderColor();
        const float scale=std::clamp(presentationScale_,0.0f,1.0f);
        const float drawWidth=s.width*scale;
        const float drawHeight=s.height*scale;
        const float drawX=p.x+(s.width-drawWidth)*0.5f;
        const float drawY=p.y+(s.height-drawHeight)*0.5f;
        if(variant_!=Variant::TEXT)
            primitives::roundedBoxRGBA(renderer,drawX,drawY,drawX+drawWidth,drawY+drawHeight,borderRadius_,bg.r,bg.g,bg.b,bg.a);
        if(variant_==Variant::OUTLINED)
            primitives::roundedRectangleRGBA(renderer,drawX,drawY,drawX+drawWidth,drawY+drawHeight,borderRadius_,border.r,border.g,border.b,border.a);
        text_->draw(renderer);
    }
    void Button::animatePressScale(float target) noexcept
    {
        if(!pressAnimationEnabled_)
        {
            cancelProperty(pressScaleProperty());
            presentationScale_=1.0f;
            return;
        }
        static constexpr float duration=0.08f;
        animateProperty(pressScaleProperty(),target,duration,AnimationEasing::EaseOut);
    }
    void Button::handleMouseDown(MouseDownEvent &e)
    {
        if(e.button!=MouseButton::Left||!isEnabled())return;
        pressed_=true;
        animatePressScale(pressScale_);
    }
    void Button::handleMouseUp(MouseUpEvent &e)
    {
        if(e.button!=MouseButton::Left)return;
        pressed_=false;
        animatePressScale(1.0f);
    }
    void Button::handleMouseClick(MouseClickEvent &e) { if(e.button==MouseButton::Left)activate(); }
    void Button::handleMouseEnter(MouseEnterEvent &) { hovered_=true; }
    void Button::handleMouseLeave(MouseLeaveEvent &)
    {
        hovered_=false;
        pressed_=false;
        animatePressScale(1.0f);
    }
    Color Button::multiplyAlpha(Color c,float f) noexcept { c.a=static_cast<uint8_t>(std::clamp(c.a*f,0.0f,255.0f)); return c; }
    Color Button::lighten(Color c,float a) noexcept { a=std::clamp(a,0.0f,1.0f); c.r=static_cast<uint8_t>(c.r+(255-c.r)*a); c.g=static_cast<uint8_t>(c.g+(255-c.g)*a); c.b=static_cast<uint8_t>(c.b+(255-c.b)*a); return c; }
}