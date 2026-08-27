#pragma once
#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <typeindex>
#include <utility>
#include <vector>
#include <SDL3/SDL.h>
#include "ui_framework/animation.hpp"
#include "ui_framework/events.hpp"
#include "ui_framework/layout_context.hpp"
#include "ui_framework/types.hpp"
namespace ui
{
    class NodeTree;
    class PanelNode;
    class LayoutSystem;
    class EventDispatcher;
    class ModalSystem;
    class Node
    {
    public:
        using Id = std::uint64_t;
        using EventHandlerId = std::uint64_t;
        using CoordinateTransform = std::function<LayoutPosition(const Node &, const LayoutPosition &)>;
        class ScopedCoordinateTransform
        {
        public:
            explicit ScopedCoordinateTransform(CoordinateTransform transform) : previous_(coordinateTransform()) { coordinateTransform() = std::move(transform); }
            ~ScopedCoordinateTransform() { coordinateTransform() = std::move(previous_); }
            ScopedCoordinateTransform(const ScopedCoordinateTransform &) = delete;
            ScopedCoordinateTransform &operator=(const ScopedCoordinateTransform &) = delete;

        private:
            CoordinateTransform previous_;
        };
        Node();
        virtual ~Node();
        Node(const Node &) = delete;
        Node &operator=(const Node &) = delete;
        Id getId() const noexcept;
        Node *getParent() const noexcept;
        void setVisible(bool);
        bool isVisible() const noexcept;
        void setEnabled(bool) noexcept;
        bool isEnabled() const noexcept;
        void setFocusable(bool) noexcept;
        bool isFocusable() const noexcept;
        void setCapturable(bool) noexcept;
        bool isCapturable() const noexcept;
        void setPosition(const LayoutPosition &);
        LayoutPosition getPosition() const noexcept;
        LayoutSize getDesiredSize() const noexcept;
        void setPositionMode(PositionMode);
        PositionMode getPositionMode() const noexcept;
        void setSize(const LayoutSizeValue &);
        LayoutSizeValue getSize() const noexcept;
        void setMinSize(const LayoutSize &);
        void setMaxSize(const LayoutSize &);
        void setMinWidth(float);
        void setMinHeight(float);
        void setMaxWidth(float);
        void setMaxHeight(float);
        LayoutSize getMinSize() const noexcept;
        LayoutSize getMaxSize() const noexcept;
        float getMinWidth() const noexcept;
        float getMinHeight() const noexcept;
        float getMaxWidth() const noexcept;
        float getMaxHeight() const noexcept;
        void setPadding(const Padding &);
        Padding getPadding() const noexcept;
        void setLeftPadding(float);
        void setRightPadding(float);
        void setTopPadding(float);
        void setBottomPadding(float);
        void setBorder(const Border &);
        Border getBorder() const noexcept;
        void setLeftBorder(float);
        void setRightBorder(float);
        void setTopBorder(float);
        void setBottomBorder(float);
        void setClipToBounds(bool) noexcept;
        bool getClipToBounds() const noexcept;
        LayoutPosition getActualPosition() const noexcept;
        LayoutSize getActualSize() const noexcept;
        virtual Node *getVisibleChild(size_t) const noexcept;
        template <typename Event>
        EventHandlerId on(std::function<void(Event &, Node &)> handler)
        {
            if (!handler)
                return 0;
            const EventHandlerId token = nextEventHandlerId();
            eventHandlers_.push_back(EventHandlerRecord{token, std::type_index(typeid(Event)), [handler = std::move(handler)](UIEvent &event, Node &node)
                                                        { handler(static_cast<Event &>(event), node); }});
            return token;
        }
        template <typename Event>
        void removeEventHandler(EventHandlerId handlerId)
        {
            const std::type_index eventType(typeid(Event));
            eventHandlers_.erase(std::remove_if(eventHandlers_.begin(), eventHandlers_.end(), [eventType, handlerId](const EventHandlerRecord &record)
                                                { return record.eventType == eventType && record.token == handlerId; }),
                                 eventHandlers_.end());
        }

    protected:
        void invalidateLayout() noexcept;
        template <typename Event>
        void emit(Event &event)
        {
            event.target = this;
            event.currentTarget = this;
            event.phase = UIEvent::Phase::TARGET;
            event.propagationStopped = false;
            std::vector<std::function<void(UIEvent &, Node &)>> callbacks;
            callbacks.reserve(eventHandlers_.size());
            const std::type_index eventType(typeid(Event));
            for (const EventHandlerRecord &record : eventHandlers_)
                if (record.eventType == eventType)
                    callbacks.push_back(record.callback);
            for (auto &callback : callbacks)
                callback(static_cast<UIEvent &>(event), *this);
        }
        FloatAnimationProperty makeFloatAnimationProperty(float &value, const void *propertyKey) noexcept
        {
            if (!propertyKey)
                return FloatAnimationProperty{};
            return FloatAnimationProperty(
                this,
                propertyKey,
                [&value]() noexcept
                { return value; },
                [&value](float next) noexcept
                { value = next; },
                [this, propertyKey, &value](float current, float target, float duration, AnimationEasing easing)
                {
                    animateFloat(propertyKey, current, target, duration, easing,
                                 [&value](float next) noexcept
                                 { value = next; });
                },
                [this, propertyKey]() noexcept
                { cancelAnimation(propertyKey); },
                animationLifetimeToken_);
        }
        bool animateProperty(const FloatAnimationProperty &property, float targetValue, float duration,
                             AnimationEasing easing = AnimationEasing::Linear) noexcept
        {
            if (!property || property.owner_ != this)
                return false;
            property.animate_(property.value(), targetValue, duration, easing);
            return true;
        }
        bool cancelProperty(const FloatAnimationProperty &property) noexcept
        {
            if (!property || property.owner_ != this)
                return false;
            property.cancel_();
            return true;
        }
        virtual void draw(SDL_Renderer *renderer) { (void)renderer; }
        virtual LayoutSize measure(const MeasureContext &context) const { return measureContent(context.availableContentSize); }
        virtual void arrange(const ArrangeContext &context) { arrangeContent(context.contentPosition, context.contentSize); }
        virtual LayoutSize measureContent(const LayoutSize &availableContent) const
        {
            (void)availableContent;
            return {};
        }
        virtual void arrangeContent(const LayoutPosition &contentPosition, const LayoutSize &contentSize)
        {
            (void)contentPosition;
            (void)contentSize;
        }
        virtual void onMount() {}
        virtual void onUnmount() {}
        virtual Node *hitTest(float, float) noexcept;

    private:
        void animateFloat(const void *, float, float, float, AnimationEasing, std::function<void(float)>);
        void cancelAnimation(const void *) noexcept;
        struct EventHandlerRecord
        {
            EventHandlerId token = 0;
            std::type_index eventType = typeid(void);
            std::function<void(UIEvent &, Node &)> callback;
        };
        static CoordinateTransform &coordinateTransform()
        {
            static thread_local CoordinateTransform transform;
            return transform;
        }
        static EventHandlerId nextEventHandlerId() noexcept
        {
            static std::atomic<EventHandlerId> next{1};
            return next.fetch_add(1, std::memory_order_relaxed);
        }
        Node *parent_ = nullptr;
        NodeTree *owner_ = nullptr;
        LayoutSizeValue size_{};
        LayoutPosition position_{};
        PositionMode positionMode_ = PositionMode::Layout;
        LayoutPosition actualPosition_;
        LayoutSize actualSize_;
        LayoutSize desiredSize_;
        LayoutSize minSize_{};
        LayoutSize maxSize_{std::numeric_limits<float>::max(), std::numeric_limits<float>::max()};
        Padding padding_;
        Border border_;
        bool clipToBounds_ = false;
        bool visible_ = true;
        bool enabled_ = true;
        bool focusable_ = false;
        bool capturable_ = false;
        std::vector<EventHandlerRecord> eventHandlers_;
        std::shared_ptr<void> animationLifetimeToken_ = std::make_shared<int>(0);
        const Id id_ = nextId();
        static Id nextId() noexcept
        {
            static std::atomic<Id> next{1};
            return next.fetch_add(1, std::memory_order_relaxed);
        }
        LayoutSize clampSize(LayoutSize, LayoutSize, LayoutSize) const;
        template <typename Event>
        void dispatchEvent(Event &event, NodeTree &nodeTree) { dispatchEventImpl(static_cast<UIEvent &>(event), std::type_index(typeid(Event)), nodeTree); }
        void dispatchEventImpl(UIEvent &, const std::type_index &, NodeTree &);
        friend class NodeTree;
        friend class PanelNode;
        friend class LayoutSystem;
        friend class EventDispatcher;
        friend class ModalSystem;
        friend class FloatAnimationProperty;
        friend class AnimationController;
    };
}
