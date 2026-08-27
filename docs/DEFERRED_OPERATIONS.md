# Deferred Operations and Phase Safety

## Purpose

Framework runtime phases must remain traversal-safe. Structural operations that would mutate the hierarchy while it is being traversed are deferred through `NodeTree` mutation handling.

## Public runtime phases

The application owns the outer loop and calls the framework through three independent operations:

```text
UIManager::processEvent(...)
    ↓
input routing and event propagation

UIManager::advanceTime(dt)
    ↓
time-dependent framework systems

UIManager::render(...)
    ↓
layout synchronization and render traversal
```

These are separate operations rather than one public `runFrame()` operation. The client does not schedule internal layout, mutation, scrolling, modality or render traversal phases individually.

When rendering uses a different logical presentation size, `render()` requests the required layout work before rendering proceeds.

## Structural operations

`PanelNode::addChild/removeChild` and root/overlay attachment/removal are routed through `NodeTree`. When a mutation scope is active, the structural change is queued instead of being applied re-entrantly.

The queue uses snapshot-swap draining. Mutations generated while a batch is executing are placed into a subsequent batch and drained by the same flush operation after the current mutation completes.

## Layout operations

`invalidateLayout()` schedules layout; it does not execute Measure/Arrange immediately.

NodeTree promotes invalidation to the containing root/overlay and deduplicates queued roots. LayoutSystem consumes the queued roots during the render-side layout synchronization phase.

A layout mutation that queues more work does not recursively restart the current traversal.

## Event operations

Input dispatch establishes a `NodeTree::ScopedMutationGuard` around event propagation. Handlers may request structural changes, but those changes are deferred until the guarded dispatch completes.

`Node::on<Event>()` registration and handler removal use a snapshot of matching callbacks for the current delivery. Changes to the live handler table therefore do not invalidate the current callback iteration.

## Time advancement

`UIManager::advanceTime(dt)` delegates time progression to `NodeTree`. `NodeTree::advanceTime()` advances framework-owned time-dependent systems; it is not a per-node `update(dt)` traversal.

A Node does not receive a universal per-frame callback. Time-dependent behavior belongs in an appropriate framework subsystem rather than being implicitly executed for every component.

## Modal and scroll operations

Modal and scroll state are framework services coordinated through `UIManager`. Their APIs do not expose internal queues or require the client to flush framework work.

Examples:

```cpp
showModal(node)
closeModal()
enableScrolling(panel)
setScrollOffset(panel, offset)
```

Scroll synchronization derives content extent from committed layout geometry. Modal synchronization validates live modal sessions and focus boundaries.

## Client responsibility

Client/component code should not attempt to flush or manually execute framework phases. `NodeTree::flushMutationQueue()` exists internally for runtime coordination and is not part of the public `UIManager` API.

There is intentionally no public API for:

```text
runLayoutNow()
runInputPhaseNow()
flushFramework()
flushScrollSystem()
flushModalSystem()
```

## Re-entrancy rule

Framework callbacks may change state and request future work. They must not assume that a structural mutation or layout consequence is committed synchronously when invoked from a guarded phase.

## General principle

> Components describe state and semantic changes; the framework controls execution timing, ordering, batching and phase safety.
