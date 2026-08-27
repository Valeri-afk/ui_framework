# Scroll System

## Role

Scrolling is framework-level infrastructure. A standalone public `Scroll` or `ScrollArea` component is not required by the current architecture.

A standard scroll container is a `PanelNode`. A plain `Node` cannot be registered as scrollable because scrolling requires descendant/content geometry.

The public entry point is `UIManager`; `ScrollSystem` remains an internal service.

## Public API

Client code interacts with scrolling through `UIManager`:

```cpp
uiManager.enableScrolling(panel);
uiManager.disableScrolling(panel);
uiManager.isScrollingEnabled(panel);

uiManager.setScrollOffset(panel, {0.0f, 100.0f});
uiManager.getScrollOffset(panel);
uiManager.getMaximumScrollOffset(panel);
```

The client does not access `ScrollSystem`, `ScrollState`, or internal content-extent calculations.

`enableScrolling()` fails when the supplied node is not a supported scroll container or is already registered.

## Ownership

The internal scroll service owns:

```text
scroll offset
content extent
maximum offset
clamping
ancestor accumulated offsets
wheel routing
layout-derived content extent
```

`UIManager` owns the service and applies the scroll coordinate transform during traversal.

## Coordinate model

Scrolling never rewrites stored layout positions.

```text
retained layout geometry
        ↓
accumulated scroll offset
        ↓
effective render/input coordinate
```

Nested scroll containers accumulate ancestor offsets.

Scroll infrastructure operates entirely in framework layout coordinates. It does not require physical window pixels or a client-provided physical viewport.

## Viewport semantics

Two viewport concepts are intentionally distinct:

```text
framework viewport
    = global UI coordinate area supplied by LayoutSystem

scroll viewport
    = current content-box geometry of the scroll container
```

The global framework viewport is derived from the renderer's logical presentation. Physical window resolution and logical presentation configuration remain renderer/client concerns.

A scroll container does not receive its viewport size from the client and does not store a duplicate viewport in `ScrollState`.

## State

`ScrollState` contains:

```text
content extent
offset
```

The viewport and maximum offset are derived from current node geometry:

```text
viewport.width  = actualWidth  - padding - border
viewport.height = actualHeight - padding - border

maxOffsetX = max(0, content.width  - viewport.width)
maxOffsetY = max(0, content.height - viewport.height)
```

Offsets are always clamped to `[0, maxOffset]`.

## Content extent

Content extent is derived from committed layout geometry.

Visible descendants contribute their actual bounds. A nested registered scroll container contributes its own viewport bounds to the parent's content calculation; its internal scrolled content does not expand the outer container's extent.

The client does not maintain a second content-size model.

When content geometry or the scroll viewport changes, the next framework synchronization recomputes the extent and reclamps the stored offset.

## Box model

The scroll viewport uses the existing Node box model. Its effective viewport is the content box after subtracting padding and border from actual size.

There is no separate Scroll-specific box model.

## Wheel routing

Wheel input is normalized by `UIManager` into framework coordinates. `UIManager` supplies the wheel position to the internal scroll service, which hit-tests and starts with the nearest registered scroll ancestor.

```text
SDL wheel
   ↓
UIManager
   ↓
ScrollSystem
   ↓
hit-test target
   ↓
nearest scrollable ancestor
   ↓
consume available delta
   ↓
clamp
   ↓
remaining delta
   ↓
next scrollable ancestor
```

A scroll container consumes only the part of the wheel delta that it can apply. If it is already at a boundary, the remaining delta can continue to an outer scroll container.

This is nested scrolling chaining; it does not introduce a second input-dispatch system.

When scrolling changes the offset, `UIManager` refreshes hover at the same pointer coordinates. Scroll does not synthesize mouse movement and does not implicitly reset pointer capture or drag state.

## Rendering and clipping

Scrolling is a coordinate transform. Stored layout geometry remains unchanged.

Clipping is a separate Node-level concern:

```cpp
node.setClipToBounds(true);
```

`clipToBounds` constrains the rendered/hit-tested subtree of that node to its own bounds.

A scroll container requires an effective viewport clip for normal scroll-container behavior, but scrolling itself is not the same concept as clipping and does not make arbitrary nodes scrollable.

The old `Overflow` enum is no longer part of the framework API.

## Scrollbars

Scrollbar visuals are outside the current scroll core. They should only be introduced after a concrete reusable visual contract exists; the current system intentionally focuses on scrolling behavior and geometry.

## Public component decision

Do not create a public Scroll component merely because a scroll service exists. Add one only when repeated application use demonstrates a reusable semantic abstraction beyond the service and ordinary `PanelNode` hierarchy.

## Regression coverage

The current regression suite covers the scroll contract as a set of integration scenarios:

```text
valid scroll-container registration
layout-derived content extent
programmatic offset and clamping
nested wheel chaining
scroll + clipToBounds + hit-test
hover refresh after scroll
content/viewport geometry change + reclamp
nested chaining in both directions
scroll-node removal and stale-state cleanup
```
