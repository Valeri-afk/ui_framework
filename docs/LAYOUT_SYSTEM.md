# Layout System

## Purpose

This document defines the current framework layout contract. The framework uses an imperative, retained-mode Measure → Arrange pipeline with explicit invalidation.

## Ownership boundary

```text
Framework
  → Measure/Arrange execution
  → constraints
  → layout scheduling
  → geometry commit
  → traversal

Component
  → component-specific Measure/Arrange policy
  → component-specific layout state

Client
  → explicit invalidation when its semantic changes make derived layout stale
```

`Node` provides the base Measure/Arrange hooks. `PanelNode` adds structural child ownership. `StackPanelNode` provides the current one-dimensional flow policy.

## Measure

Measure receives an available content-space proposal after framework border/padding conversion and constraint handling. The component returns desired content size; the framework composes it back into desired border-box size and applies final constraints.

```text
parent proposal
    ↓
framework proposal/constraint resolution
    ↓
content-box proposal
    ↓
component Measure
    ↓
desired content size
    ↓
framework border-box composition + final constraints
    ↓
desired size
```

A maximum can narrow the measurement proposal. A minimum is applied to final size and does not automatically become a narrower measurement proposal.

## Arrange

Arrange receives the final content position and size selected by the parent layout policy. The framework commits `actualPosition` and `actualSize` as derived geometry.

```text
parent allocation
    ↓
component Arrange
    ↓
child placement
    ↓
framework final-size constraints
    ↓
actual geometry
```

`Measure proposal != Arrange allocation` is a core invariant.

## Constraint semantics

The current `LayoutValue` model has only:

```text
Auto
Value (fixed)
```

Minimum and maximum size are separate Node constraints.

Current behavior is:

```text
fixed size → measurement proposal and final-size constraint
max size   → measurement proposal bound and final-size constraint
min size   → final-size constraint
auto       → surrounding layout policy / intrinsic measurement
```

`Auto` does not itself mean fill-parent.

## Border-box model

Node outer geometry is a border box. Component Measure/Arrange hooks operate on content-space values; the framework converts between border-box and content-box using sanitized padding and border values.

There is no `Overflow` layout enum. Clipping is controlled by the Node property:

```cpp
node.setClipToBounds(true);
```

## Linear layout

`StackPanelNode` supports:

```text
Vertical / Horizontal orientation
gap
main-axis alignment: START / CENTER / END / SPACE_BETWEEN
cross-axis alignment: START / CENTER / END / STRETCH
```

Visible non-absolute children participate in normal flow. Absolute children are measured/arranged separately and do not contribute to normal-flow aggregation.

Stretch expands the cross axis before final min/max constraints are applied. Flex grow/shrink/basis/wrap and grid are not implemented.

## Absolute positioning

`PositionMode::Absolute` removes a child from normal linear flow. The parent layout still measures and arranges that child separately using the child's position relative to the parent's content position and the applicable size constraints.

## Invalidation

The public invalidation entry point is:

```cpp
uiManager.invalidateLayout(node);
```

`Node::invalidateLayout()` also routes through its owning `NodeTree` for mounted nodes.

Invalidation is explicit and asynchronous. It does not run Measure/Arrange immediately and there is no public flush operation.

The current NodeTree implementation promotes a descendant invalidation to its containing top-level root/overlay and deduplicates the queued root. The next `LayoutSystem::processLayoutQueue()` pass performs the complete recursive Measure/Arrange operation for that root.

## Layout scheduling and frame order

`UIManager::runFrame()` currently performs layout before node update:

```text
sync input/modal state
    ↓
flush pending tree mutations
    ↓
process layout queue
    ↓
scroll synchronization
    ↓
modal synchronization/update
    ↓
NodeTree update
    ↓
draw
```

If the renderer's logical presentation size changes, `LayoutSystem` requests a full layout for all roots/overlays before the frame's layout processing.

## Re-invalidation

If Measure/Arrange causes another invalidation while a layout pass is active, the mutation/invalidation is not used to recursively restart the current traversal. The newly queued work is handled by a later framework-controlled layout pass.

## Derived geometry validity

`getDesiredSize()` and `getActualSize()` expose cached derived layout state. After invalidation and before the next layout pass, previously committed geometry may be stale.

## Structural interaction

`PanelNode::addChild/removeChild` already route mounted structural changes through `NodeTree`. They establish ownership/liveness and queue the affected parent/root for layout as part of the structural operation. No separate `treeStructureChanged()` notification exists.

## No paint invalidation

Rendering runs every frame. There is no separate public `invalidatePaint()` queue.

## Acceptance cases

Important cases include fixed-size nodes, text width-sensitive measurement, padding/border conversion, min/max constraints, wrapping after parent width changes, main/cross-axis alignment, absolute children, hidden children, nested panels and overflowing content.

## Intentionally deferred

```text
flex-grow
flex-shrink
flex-basis
flex-wrap
order
margin
Grid track sizing
multi-pass intrinsic track resolution
content-dependent stretch remeasurement
```
