# Rendering System

## Role

Rendering is framework runtime infrastructure below the component layer.

```text
Node/component
   ↓
component visual state
   ↓
rendering primitives / specialized backend
   ↓
SDL renderer
```

Components draw their own visual content. `NodeTree` owns render traversal, ordering, clipping and traversal safety.

## Runtime entry point

The application invokes rendering through `UIManager::render()`. Rendering is an independent framework phase; it is not coupled to a public per-node `update()` phase.

Conceptually:

```text
UIManager::render()
       ↓
layout synchronization
       ↓
scroll/modal synchronization
       ↓
NodeTree render traversal
       ↓
Node/component draw hooks
```

The outer application loop remains client-owned. The framework owns the work performed after `render()` is invoked.

## SDL logical presentation

The chess client uses a fixed logical UI space:

```text
1920 × 1080
```

The renderer is configured with:

```cpp
SDL_SetRenderLogicalPresentation(
    renderer,
    1920,
    1080,
    SDL_LOGICAL_PRESENTATION_LETTERBOX);
```

The framework reads the current logical presentation from the renderer during `render()` and uses the resulting size as its layout viewport. Physical window/output dimensions are not the framework UI coordinate system when logical presentation is active.

## Renderer state isolation

Rendering code preserves and restores mutable renderer state where required. The framework contains an internal `RendererStateScope` for target, viewport, scale, clip, draw color, blend mode and color-scale state.

## Clipping

`Node::setClipToBounds(true)` is the current Node-level clipping contract. It is a rendering/input boundary, not a layout constraint.

Conceptually:

```text
current parent clip
    ∩
node clip
    ↓
draw node
    ↓
draw descendants
    ↓
restore renderer state
```

`PanelNode` does not own recursive rendering traversal. `NodeTree` does.

There is no `Overflow` enum in the current public API.

## Scroll transform

Scrolling does not rewrite retained layout geometry. `ScrollSystem` supplies an accumulated offset through the framework's coordinate-transform mechanism during rendering and input hit testing.

```text
stored layout position
    ↓
accumulated scroll transform
    ↓
effective render/input position
```

Nested scroll containers accumulate their registered ancestor offsets. Clipping and scrolling remain separate mechanisms: clipping limits the visible/interactive region while scrolling changes effective descendant coordinates.

## Image rendering

`Image` is a thin component over an externally owned `SDL_Texture`.

```text
Image visual state
    ↓
fit geometry + tint
    ↓
SDL_RenderTexture
```

The component does not own or destroy the texture. `STRETCH`, `CONTAIN` and `COVER` affect presentation inside the arranged box and do not establish texture/resource ownership.

## Rendering primitives

Low-level primitives remain below component semantics. They provide reusable immediate SDL drawing operations such as rectangles, rounded rectangles, lines and circles.

A primitive does not own Node lifecycle, input, focus, modality, scrolling or application semantics.

## Non-rectangular geometry boundary

The current framework uses rectangular bounds for Measure/Arrange and base hit testing. Rounded rectangles are already available as a component/presentation primitive (for example `Button::setBorderRadius()`), but there is not yet a general shape-aware hit-testing/clipping model for arbitrary non-rectangular geometry.

Future shape support should preserve this separation:

```text
Layout
  → rectangular bounds

Shape presentation / interaction
  → component or future framework geometry contract
```

Do not introduce a general geometry abstraction until a concrete reusable shape requirement exists.

## Transform boundary

Scale and rotation are not framework-wide layout semantics. The current framework does have a narrow coordinate-transform hook used internally by scrolling; this is not a public general transform stack.

A future global transform model would need consistent drawing, hit testing, clipping, transform origin and scroll ordering semantics.

## Text rendering

Text rendering is a dedicated internal backend:

```text
TextContent
   ↓
TextLayout / TextRenderer
   ↓
SDL_ttf / SDL renderer
```

`TextRenderer` owns backend-oriented rasterization and renderer integration. Logical text measurement/wrapping and editing semantics remain outside it.

## Rendering cadence

Rendering executes whenever the application invokes `UIManager::render()`. There is no public paint invalidation queue.

Render-only state may change without forcing Measure/Arrange. A dedicated paint-dirty API should only be introduced if the rendering architecture later requires explicit scheduling.

## Validation

The repository contains visual/integration coverage for typography, text alignment/wrapping, text-bearing controls, button variants, menus/dropdowns, tabs, stack/scroll presentation, modal/backdrop presentation, Image fit behavior and 1920×1080 logical presentation.
