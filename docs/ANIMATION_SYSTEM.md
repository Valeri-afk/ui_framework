# Animation System

The UI framework provides one small property-based animation runtime shared by framework components and client code. It is intentionally a transition runtime rather than a general-purpose animation authoring system.

## Runtime model

Animations advance through the normal framework time phase:

```text
UIManager::advanceTime(dt)
        ↓
NodeTree::advanceTime(dt)
        ↓
AnimationSystem::advance(dt)
```

`render()` presents the current state. It does not advance animations.

## Public entry point

Client code reaches the runtime through `UIManager::animations()`:

```cpp
ui.animations().to(
    button.pressScaleProperty(),
    0.92f,
    0.12f,
    ui::AnimationEasing::EaseOut);

ui.animations().cancel(button.pressScaleProperty());
```

The public API works with `FloatAnimationProperty` handles. Client code does not construct runtime animation objects and does not need to know how the property is stored or updated.

The generic easing default is `Linear`. Components should explicitly select an easing curve when it is part of their visual behavior.

## Property model

The current primitive animates a `float` property:

```text
current value → target value
```

A property exposes a value getter/setter and a framework-owned transition callback. A property handle is valid only while its owning Node is alive.

Properties are intentionally explicit capabilities. A component publishes only presentation values that make sense as part of its public behavior. Internal semantic state does not become animatable merely because it is represented by a field.

Only one active transition exists for a given property on a Node. Starting another transition for the same property replaces the previous transition and samples the property's current value immediately before the replacement starts.

Different properties on the same Node can animate concurrently.

## Component behavior

Components are allowed to initiate transitions for their own presentation state. They do not own a second animation system.

The boundary is:

```text
component
    ↓
chooses when and why a visual transition occurs
    ↓
FloatAnimationProperty
    ↓
shared AnimationSystem
```

This lets default component behavior remain encapsulated while still allowing a component to expose selected presentation properties to client code.

For example, `Button` owns the semantic `pressed` state and the private `presentationScale` value. Its default press/release behavior drives `presentationScale` through the same property/runtime path exposed by `pressScaleProperty()`.

The client can therefore override or replace the transition of that presentation value without knowing the Button's internal field layout.

## Completion and cancellation

An animation with zero or negative duration applies its target immediately.

Cancelling an animation removes only the transition. The property's current value is preserved.

When a transition reaches its target, the target value is written and the transition is removed.

If the property owner is destroyed, its property handle becomes invalid and the runtime no longer updates that property.

## Easing

The runtime supports:

```text
Linear
EaseIn
EaseOut
EaseInOut
```

The animation runtime owns interpolation mechanics. The caller chooses easing when it matters to the requested transition.

## Ownership and lifetime

`AnimationSystem` is owned by `NodeTree`.

An animation never owns its target Node. `FloatAnimationProperty` keeps only non-owning access to the property together with a lifetime token owned by the Node.

This prevents a dead Node from being updated after destruction.

## Current consumers

### Button

`Button` uses a private presentation scale for press/release feedback. Its default behavior remains component-owned, while `pressScaleProperty()` provides a controlled public animation capability.

The semantic `pressed` state and layout size are not themselves animation properties.

### Modal backdrop

`ModalSystem` animates the opacity of its internal backdrop Node using the same runtime. Modal session state and backdrop presentation state remain separate.

## Deliberate non-goals

The framework does not currently provide:

```text
public Animation object lifecycles
animation timelines/sequences as a separate authoring layer
animation callbacks such as onStart/onEnd
universal registration of arbitrary component fields
universal animated-state configuration on Node
implicit animation of every property
animation of rasterized text by changing font size
```

These should only be added when a concrete framework or client requirement demonstrates that the property-based transition runtime is insufficient.
