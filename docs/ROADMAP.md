# UI Framework Roadmap

## Status

This is a global checklist, not a strict implementation sequence. The source code is authoritative; focused subsystem documents describe current contracts.

## Core completion checklist

### Lifecycle

- [x] Separate external event processing, time advancement and rendering into independent `UIManager` operations.
- [x] Remove the universal per-Node `update(dt)` lifecycle.
- [x] Keep `NodeTree::advanceTime(dt)` limited to explicitly time-dependent framework systems.
- [x] Keep `Node::draw()` as an internal rendering hook while exposing `UIManager::render()` as the public rendering phase.

Reference: `FRAMEWORK_SCOPE.md`, `RENDERING_SYSTEM.md`

### Animation system

- [x] Add a small internal float property animation runtime owned by `NodeTree`.
- [x] Support multiple animated properties on one Node.
- [x] Replace an active transition when the same property is animated again.
- [x] Support cancellation and zero-duration transitions.
- [x] Protect animation targets from destroyed Nodes through the Node lifetime mechanism.
- [x] Use animation for Button press presentation scale.
- [x] Use animation for ModalSystem backdrop opacity.
- [x] Keep Animation objects out of component/client ownership.
- [x] Keep universal animation configuration, callbacks and property registration out of the current contract.

Reference: `ANIMATION_SYSTEM.md`

### Scroll system

- [x] Implement and validate scroll-container registration and state.
- [x] Validate layout-derived content extent and offset clamping.
- [x] Validate nested scrolling and remaining wheel-delta propagation.
- [x] Validate scroll transforms together with clipping, hit testing and hover refresh.
- [x] Keep scrolling as internal `ScrollSystem` infrastructure exposed through `UIManager` rather than introducing a public scroll component.

Reference: `SCROLL_SYSTEM.md`

### Text input

- [x] Implement single-line editable text input.
- [x] Integrate committed text and IME events with existing `InputSystem` focus and keyboard routing.
- [x] Build editing on top of `TextContent` / `TextLayout` / `TextRenderer` without moving editing semantics into `Node` or the renderer.
- [x] Implement caret, selection, keyboard editing, mouse caret positioning and drag selection.
- [x] Add focused regression coverage for text editing, Unicode text input, selection, focus lifecycle and IME composition.
- [ ] Add clipboard support, richer IME/window integration, multiline editing, undo/redo, word-wise navigation or a text viewport only when a concrete reusable requirement exists.

Reference: `TEXT_INPUT_SYSTEM.md`

### Modality

- [x] Validate modal open/close sequencing.
- [x] Validate focus restoration and fallback focus selection.
- [x] Validate backdrop behavior and modal interaction boundaries.
- [x] Validate nested modal sessions and focus trapping where supported.
- [x] Keep modality as `ModalSystem` infrastructure exposed through `UIManager`; no standalone public `Modal` component is currently required.

Reference: `MODALITY_SYSTEM.md`, `ANIMATION_SYSTEM.md`

### Text architecture boundary

- [x] Keep `TextLayout`, `TextContent` and internal `TextRenderer` responsibilities separated.
- [x] Keep logical text measurement/wrapping separate from backend rasterization while retaining the current SDL_ttf dependency.
- [x] Keep editing state in `TextInput`/`TextEditState`, not in `Node` or `TextLayout`.

Reference: `TEXT_SYSTEM.md`

### Base and standard components

- [x] Maintain the current standard component set required by the target application.
- [x] Use the existing Node event-registration model for component handlers.
- [x] Keep component-specific semantics in components while lifecycle, traversal, layout, input, rendering, modality and scrolling remain framework-owned.
- [x] Keep `Image` as a thin component with non-owning `SDL_Texture*` semantics.

Reference: `COMPONENT_DESIGN.md`, `EVENT_DISPATCHING.md`

## Layout and geometry

### Border-box model

- [x] Stabilize border-box/content-box conversion.
- [x] Verify desired size, arranged size and rendered geometry remain consistent.

Reference: `LAYOUT_SYSTEM.md`

### Clipping

- [x] Validate `Node::clipToBounds` semantics.
- [x] Validate nested clipping intersections.
- [x] Validate clipping consistency between rendering and hit testing.
- [x] Validate clipping together with scrolling transforms.

Reference: `RENDERING_SYSTEM.md`, `INPUT_SYSTEM.md`, `SCROLL_SYSTEM.md`

### Absolute positioning

- [x] Verify `PositionMode::Absolute` behavior.
- [x] Verify absolute children are excluded from normal linear flow.
- [x] Verify their final geometry and interaction boundaries.

Reference: `LAYOUT_SYSTEM.md`

### LayoutValue type

- [x] Stabilize the current `Auto` / fixed `Value` model.
- [x] Verify fixed, min and max behavior against Measure / Arrange contracts.

Reference: `LAYOUT_SYSTEM.md`

### Non-rectangular geometry

- [ ] Define framework-wide semantics for rounded rectangles, circles and other non-rectangular interaction/rendering shapes.
- [ ] Define shape-aware clipping and hit testing without leaking renderer-specific details into `NodeTree`.
- [ ] Define the minimum reusable gameplay UI shape set before introducing a general geometry abstraction.

Reference: `LAYOUT_SYSTEM.md`, `INPUT_SYSTEM.md`, `RENDERING_SYSTEM.md`

### Transform geometry

- [ ] Decide whether scale/rotation belong in framework-wide geometry semantics.
- [ ] If transforms become framework-wide, define consistent drawing, hit testing, clipping, inverse input transforms, origin/pivot and scroll ordering.
- [ ] Do not turn the existing internal scroll coordinate-transform hook into a public general transform stack without a concrete requirement.

Reference: `LAYOUT_SYSTEM.md`, `INPUT_SYSTEM.md`, `RENDERING_SYSTEM.md`, `ARCHITECTURE.md`

## NodeTree and component structure

### NodeTree ownership

- [x] Keep `Node::owner_` as the node's runtime membership and mutation-routing reference.
- [x] Preserve live-node registration and ownership invariants.

Reference: `LIFETIME_AND_MUTATIONS.md`, `ARCHITECTURE.md`

### PanelNode / structural mutation boundary

- [x] Use `PanelNode::addChild/removeChild` as the supported structural API.
- [x] Route mounted structural mutation through `NodeTree`.
- [x] Preserve ownership, registration, lifecycle and deferred-mutation invariants.

Reference: `COMPONENT_DESIGN.md`, `LIFETIME_AND_MUTATIONS.md`, `DEFERRED_OPERATIONS.md`, `ARCHITECTURE.md`

## Cross-system validation

- [x] Validate input/event behavior after structural changes.
- [x] Validate modal restrictions with focus, capture and overlays.
- [x] Validate scroll behavior with clipping, hit testing and hover.
- [x] Validate layout invalidation followed by the next layout pass.
- [x] Validate rendering after geometry changes and logical-presentation resize.

## Architectural guardrails

- [x] Keep `UIManager` as the public framework facade.
- [x] Keep `NodeTree` as the runtime authority for ownership, liveness, structural mutation and coordinated traversal.
- [x] Keep `ScrollSystem` and `ModalSystem` as internal services.
- [x] Keep animation runtime internal to the framework; components request property transitions rather than owning animation objects.
- [x] Do not introduce a universal property/dependency system without a concrete requirement.
- [x] Do not introduce parallel input, event or rendering orchestration systems in components/client code.
- [x] Avoid unnecessary abstractions until a repeated reusable requirement proves their value.

## Large-file safety

`node_tree.cpp` and `input_system.cpp` are large implementation files and should not be partially rewritten by automated edits. Changes requiring edits to those files should be performed manually and verified against the current source before being committed.
