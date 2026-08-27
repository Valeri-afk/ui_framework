# Event Dispatching

## One event mechanism

The framework uses one public registration mechanism on `Node`:

```cpp
node->on<ConcreteEvent>(callback);
```

`events.hpp` is the public event-type header. Handler storage and the propagation dispatcher are framework internals.

## Concrete event types

The template parameter selects the event type at registration time:

```cpp
node->on<ui::MouseClickEvent>(
    [](ui::MouseClickEvent& event, ui::Node& node) {
        // custom behavior
    });
```

Callbacks receive the concrete event and the current `Node`.

Current framework input/lifecycle events include:

```text
MouseMoveEvent
MouseDownEvent
MouseUpEvent
MouseClickEvent
MouseWheelEvent
MouseEnterEvent
MouseLeaveEvent
MouseDragBeginEvent
MouseDragEvent
MouseDragEndEvent
KeyDownEvent
KeyUpEvent
FocusGainedEvent
FocusLostEvent
TextInputEvent
TextEditingEvent
```

Components also define semantic events such as `ButtonActivatedEvent`, `CheckboxToggledEvent`, `RadioButtonSelectedEvent`, `SliderValueChangedEvent`, `TextChangedEvent` and `TextInputSubmittedEvent` where their public contracts require them.

## Framework flow

For ordinary input:

```text
SDL input
  ↓
UIManager / InputSystem
  ↓
hit test / focus / capture / modality
  ↓
concrete framework event
  ↓
EventDispatcher
  ↓
registered Node handlers
```

Wheel events are first offered to the internal `ScrollSystem`; a handled wheel operation refreshes hover and does not continue as an ordinary input dispatch.

## Propagation

`EventDispatcher` constructs an ancestry path from target to root using `NodeId` values and resolves each node again through `NodeTree` during dispatch.

The caller chooses whether tunneling and bubbling are enabled:

```text
TUNNELING
    root → ... → target

TARGET
    target

BUBBLING
    parent → ... → root
```

`UIEvent` exposes:

```text
target
currentTarget
phase
propagationStopped
```

`stopPropagation()` stops further propagation through subsequent nodes/phases. It does not remove other handlers already selected for the current target delivery.

If a node is removed during dispatch, later propagation steps resolve against `NodeTree`; a node that is no longer live is not dispatched to.

## Handler snapshots

`Node` copies matching callbacks into a stable snapshot before invoking them. A handler added or removed while the current snapshot is executing does not invalidate that iteration.

## Component vs client handlers

The same registration mechanism supports both component-internal behavior and client customization. Components may register private handlers for their default interaction semantics; clients can register their own handlers through the same API.

## Semantic events

Semantic component events are emitted by the component after a meaningful state/action transition. They do not require a second event bus or changes to `InputSystem`.

Examples:

```text
Button      → ButtonActivatedEvent
Checkbox    → CheckboxToggledEvent
RadioButton → RadioButtonSelectedEvent
Slider      → SliderValueChangedEvent
TextInput   → TextChangedEvent / TextInputSubmittedEvent
```

## Registration lifetime

Handler registration belongs to the Node/component lifetime. The registration token returned by `Node::on<Event>()` can be used with the corresponding typed removal API.

## Constructor caution

A component constructor may register internal handlers when that is part of its runtime behavior. Registration itself must not simulate a user interaction.

## Non-goals

```text
second event bus
manual client-side NodeTree traversal for dispatch
client access to handler storage
generic runtime event casting
component-specific global dispatch systems
```
