# UI Framework

C++20 UI framework built on SDL3.

The repository contains the current framework source code and the focused architectural contracts used during development.

## Documentation

Start with:

- [Framework Scope](docs/FRAMEWORK_SCOPE.md)
- [Layout System](docs/LAYOUT_SYSTEM.md)
- [Lifetime and Mutations](docs/LIFETIME_AND_MUTATIONS.md)
- [Deferred Operations](docs/DEFERRED_OPERATIONS.md)
- [Component Design](docs/COMPONENT_DESIGN.md)
- [Input System](docs/INPUT_SYSTEM.md)
- [Event Dispatching](docs/EVENT_DISPATCHING.md)
- [Rendering System](docs/RENDERING_SYSTEM.md)
- [Modality System](docs/MODALITY_SYSTEM.md)
- [Scroll System](docs/SCROLL_SYSTEM.md)
- [Text System](docs/TEXT_SYSTEM.md)
- [Text Input System](docs/TEXT_INPUT_SYSTEM.md)
- [Animation System](docs/ANIMATION_SYSTEM.md)
- [Architecture](docs/ARCHITECTURE.md)
- [Development Instructions](docs/INSTRUCTIONS.md)

`ARCHITECTURE.md` is the large current-architecture document and is maintained separately from the focused subsystem contracts.

The source code is the authoritative source of truth for current behavior. The focused documents describe stable contracts, ownership boundaries, deliberate non-goals and future boundaries; they are not phase checkpoints.

## Current state

The framework currently provides retained-node ownership and lifecycle, deferred structural mutation, Measure → Arrange layout with explicit invalidation, SDL input routing, event dispatch, focus/capture, rendering/clipping, modality, scrolling, text rendering/layout, a small property-based animation runtime with a public `UIManager::animations()` facade, and a single-line game-oriented `TextInput` control.

Animation is intentionally property-based rather than a separate object/timeline system. Components keep semantic state separate from presentation state and expose only presentation values that are meaningful animation capabilities.

The framework remains intentionally small. Advanced geometry, a general resource manager, richer text editing, scrollbar presentation and standalone `Scroll` / `ScrollArea` or `Modal` components remain outside the current stabilized scope.
