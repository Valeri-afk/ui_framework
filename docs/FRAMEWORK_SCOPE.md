# Framework Scope and Purpose

## Why this framework exists

The project originated from a chess application, but the UI framework is not a chess framework and is not intended to become a complete widget library.

It provides a small retained-mode C++/SDL3 runtime in which independently implemented UI objects participate in one coherent system with shared ownership, lifecycle, traversal, layout, input, events and rendering.

The chess application is the immediate validation target, not the source of application-specific framework components.

## What the framework is

The framework provides generic infrastructure for:

- hierarchical UI ownership and lifetime;
- lifecycle and safe traversal;
- deferred structural mutation;
- event-driven input and interaction;
- input coordination, focus, capture and hit testing;
- event dispatching;
- Measure → Arrange layout with explicit invalidation;
- scrolling and modality as framework services;
- low-level rendering primitives;
- a small standard component set;
- time advancement for framework systems that are explicitly time-dependent;
- custom `Node` and `PanelNode` extension points;
- single-line game-oriented text input/editing.

It is intentionally narrower than Qt, WPF or a universal application toolkit.

## Runtime model

The framework participates in an execution loop owned by the application. It does not own the application's main loop and does not impose a game-loop policy on clients.

The public runtime boundary consists of three independent operations:

```text
processEvent(event)
        ↓
handle external input and dispatch UI events

advanceTime(dt)
        ↓
advance explicitly time-dependent framework systems

render(renderer)
        ↓
perform layout synchronization and rendering traversal
```

Application code may call these operations from whatever outer loop is appropriate for the application. A game may call `advanceTime(dt)` every iteration; an event-driven desktop application may use it only when time-dependent UI behavior is active. Rendering cadence is likewise an application concern, while the framework owns render traversal when `render()` is invoked.

Application code may mutate retained objects directly:

```cpp
button->setVisible(false);
panel->addChild(...);
button->setPadding(...);
```

These operations describe changes to retained semantic state or structure. They do not give client code control over framework phase ordering. `UIManager`, `NodeTree` and the internal subsystems remain responsible for lifecycle, mutation safety, scheduling, layout execution, input routing and rendering traversal.

## Developer vs framework responsibility

The framework owns:

```text
lifecycle
runtime invariants
ownership / live-node state
tree integration
traversal
scheduling
layout execution
input routing
event propagation
render traversal
service coordination
time advancement of framework-managed systems
```

A component/client owns:

```text
component-specific state
application-specific meaning
custom Measure/Arrange/Draw behavior
semantic actions/callbacks
explicit notifications when framework-derived state may be stale
```

The developer should be able to describe semantic state and provide component behavior without taking over the runtime control mechanisms.

## Application boundary

```text
Chess Engine / Domain
        ↓
Chess Client / Application
        ↓
UI Framework
```

The chess engine owns chess state and rules. The client owns application meaning and behavior. The framework owns reusable UI runtime mechanisms and standard generic UI concepts.

## Framework responsibilities

The framework owns coordinated mechanisms required to make many UI objects operate as one runtime:

```text
runtime structure
  ownership / lifetime / registration / traversal / mutation safety

layout
  Measure / Arrange / constraints / scheduling / geometry commit

interaction
  input routing / focus / capture / hit testing / event propagation

rendering
  render traversal / clipping / ordering / renderer-state safety

time-dependent services
  explicit advanceTime() processing for framework-owned behavior

services
  modality / scrolling
```

Custom components should use these mechanisms instead of implementing competing runtime systems.

## Client responsibilities

The client owns application-specific state and meaning, for example:

- chess rules and engine integration;
- game state and clocks;
- navigation intent;
- application-specific semantics and callbacks;
- custom component behavior;
- ownership of the outer application loop.

## Current standard components

```text
Button
ToggleButton
Menu / MenuItem
TabControl / TabItem
Checkbox
RadioButton
Slider
Dropdown
Typography
TextInput
Image
StackPanelNode / PanelNode
```

The framework remains intentionally minimal. `Paper`, `Label` and `Card` are composition/styling patterns rather than mandatory framework components.

## Infrastructure vs component

Before adding a component, determine whether the required behavior is actually infrastructure:

```text
layout calculation
child hit-testing
common event dispatch
input routing
focus/capture
modality
scroll coordination
```

A component belongs in the framework when it is a generic reusable UI concept with a clear contract. Application-specific composition stays in client code.

Use `Node` by default. Use `PanelNode` only when structural children and child layout are part of the component's semantics. `StackPanelNode` should be reused when its linear layout policy matches the required behavior.

## Current service decisions

### Modality

Modality is framework infrastructure behind the public `UIManager` facade. A standalone public `Modal` component is not currently required.

### Scrolling

Scrolling is framework infrastructure behind the public `UIManager` facade. `ScrollSystem` is an internal service; a standalone public `Scroll` / `ScrollArea` component is not currently required.

### Text

The active text architecture is:

```text
Typography / text-bearing controls
        ↓
    TextContent
      ↙     ↘
 TextLayout  TextRenderer
```

`TextLayout` provides logical measurement/wrapping using SDL_ttf font metrics. `TextContent` bridges component presentation and geometry to the layout/rendering layers. `TextRenderer` is internal backend rendering. Source `TTF_Font*` remains client-owned; derived renderer resources are framework-owned.

`TextInput` is an active single-line editing component. It owns committed text, caret/selection state and private IME composition state while using `TextContent` for layout/rendering geometry.

## Ownership and lifetime

The framework uses `std::unique_ptr` as the fundamental structural ownership mechanism.

Client-held `Node*` references are non-owning. Live membership is authoritative in `NodeTree` behind `UIManager`.

Structural mutations are framework-managed and deferred when required for traversal safety.

For text, the source `TTF_Font*` is non-owning from the framework perspective and must outlive its text users. No general ResourceManager is currently required by the framework contract.

## Reparenting

Reparenting is a future capability, not a current requirement. It should only be introduced after a concrete use case establishes the need.

## Design philosophy

1. Runtime correctness before component breadth.
2. Client extensibility without losing framework runtime control.
3. Minimal sufficient public API.
4. Concrete requirements before abstractions.
5. Application/domain logic stays outside the framework.
6. Components do not reimplement global runtime mechanisms.
7. The source code is authoritative for current behavior.
8. Documentation describes stable contracts and intentional future boundaries, not historical phase checkpoints.
9. Do not introduce a universal property or dependency system unless concrete requirements demonstrate that the existing explicit contracts are insufficient.

## Why there is no universal property/dependency system

The framework intentionally does not observe arbitrary component fields or infer a global dependency graph.

It does not currently introduce:

```text
universal property registration
property metadata/dependency graph
dynamic property maps
automatic observation of arbitrary fields
signals/change tracking as a global requirement
tree diffing/reconciliation
```

Framework-known state is handled by explicit framework contracts, while component-owned state remains local and participates through explicit hooks and notifications.

## Future capability rule

A capability should be added when it is a real reusable responsibility of the UI runtime or is repeatedly required by the supported application class, and when its developer contract remains appropriately small.

Deferred examples include:

```text
clipboard support
rich IME/window integration
multiline text editor
undo/redo and word-wise text navigation
text viewport/scrolling inside TextInput
scrollbar presentation
standalone Scroll / ScrollArea component
standalone Modal component
reparenting
advanced geometry/transforms
```

These are requirements to evaluate, not commitments to implement blindly.

## Documentation roles

`FRAMEWORK_SCOPE.md` defines why the framework exists, what belongs in it, and the intended developer/framework boundary.

`INSTRUCTIONS.md` defines repository-analysis, implementation-safety and documentation workflow rules.

`ARCHITECTURE.md` remains the large current-architecture document and is maintained separately from this focused scope document.

`README.md` is the entry point to the focused subsystem contracts.
