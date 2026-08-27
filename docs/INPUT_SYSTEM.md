# Input System

## Role

Input is framework runtime infrastructure. Client code and components consume framework events; they do not reimplement global routing, hit testing, pointer capture, focus or scroll routing.

## Main responsibilities

The input system owns:

```text
SDL event ingestion
pointer coordinate normalization
hit testing
hover state
pressed/drag state
pointer capture
focus
keyboard routing
modal-root filtering
framework event generation
```

Wheel scrolling is coordinated by `UIManager` and the internal `ScrollSystem`, which runs before ordinary `InputSystem::processEvent()` handling for wheel events.

## Coordinate normalization

`UIManager::processEvent()` converts SDL events into renderer/framework render coordinates when a renderer is supplied. The chess client uses a fixed logical presentation of:

```text
1920 × 1080
```

with `SDL_LOGICAL_PRESENTATION_LETTERBOX`.

Physical window dimensions may differ. Framework hit testing and dispatch operate in the resulting logical/render coordinate space.

Scrolling then applies its own accumulated coordinate transform without rewriting retained layout positions.

## Hit testing and clipping

Hit testing uses retained geometry transformed by the active internal scroll coordinate transform. `Node::setClipToBounds(true)` is the Node-level clipping/interaction boundary.

The current hit-test traversal follows effective paint order, searches overlays/roots with higher-priority entries first, and recursively tests visible/enabled children. When `clipToBounds` is enabled, descendant interaction is constrained by the node's bounds.

There is no public `rebuildHitTest()` operation and no `Overflow` API.

## Hover

Pointer movement updates the hovered target and generates enter/leave behavior.

After `ScrollSystem::handleWheel()` handles any wheel delta, `UIManager` refreshes hover using the same pointer coordinates and the updated scroll transform. Scrolling does not synthesize a mouse-move event or implicitly reset pointer capture.

## Pointer interaction

The input state vocabulary includes:

```text
pressed node
captured node
focused node
dragging state
pointer position
```

Pointer capture is framework-owned so a captured control can continue receiving movement/release events outside its ordinary hit-test region.

The default drag threshold is `5.0f`.

When a modal opens, incompatible pointer interaction outside the modal boundary is cancelled.

## Focus

Focus is framework state. `InputSystem` validates focus against liveness, visibility, enabled state, focusability and the active modal boundary.

Focus transitions dispatch `FocusLostEvent` and `FocusGainedEvent`. Focus transitions are reentrancy-aware; requests made from focus callbacks are reconciled by the input system.

When a modal opens, modality establishes the focus scope. The modal itself receives focus when focusable; otherwise the first valid focusable descendant is selected. `Tab` traversal is implemented by `ModalSystem` inside the active modal subtree.

## Modal filtering

When a modal root is active, normal input targeting is restricted to the active modal subtree. Outside pointer input is handled by `ModalSystem` according to `OutsideClickBehavior`.

For Escape/Tab, `UIManager` gives the key event to the focused node first and then lets `ModalSystem` apply the active modal policy if the event was not stopped.

## Wheel

Wheel input is handled by the internal `ScrollSystem` before ordinary input dispatch:

```text
SDL wheel
   ↓
UIManager
   ↓
ScrollSystem
   ↓
hit-test target
   ↓
nearest registered scroll ancestor
   ↓
consume/clamp delta
   ↓
remaining delta → outer scroll ancestor
```

A scroll container consumes only the delta it can apply. Remaining delta may chain to an outer registered scroll container.

A handled scroll refreshes hover but does not change retained layout geometry or pointer capture state.

## Event generation

`InputSystem` translates SDL input into framework events including:

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

`TextInputEvent` carries committed SDL text input. `TextEditingEvent` carries temporary IME composition data. Dispatch propagation is a separate concern described by `EVENT_DISPATCHING.md`.

## Validation boundary

Regression suites cover pointer sequencing, hover, capture, focus, keyboard routing, dragging, modal filtering, scroll/hit-test interaction, hover refresh after scroll, nested wheel chaining, text input routing and text-input focus isolation.

Visual rendering validation remains separate from input correctness validation.

## Non-goals

Do not add a second client-side input routing system or expose internal input phase controls through the public API.
