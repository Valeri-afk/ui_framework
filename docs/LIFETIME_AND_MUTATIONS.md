# Lifetime and Mutations

## Runtime ownership

The retained UI runtime has one authoritative tree owner: `NodeTree`, behind the public `UIManager` facade.

```text
UIManager
  ↓
NodeTree
  ↓
roots / overlays / live nodes
```

`NodeTree` owns live-node membership, ownership/registration, traversal safety, structural mutation handling, and layout queue semantics.

A node is only a framework-live node while it is owned by the active tree.

## Node lifetime

Client code creates nodes and transfers ownership into the framework through root/overlay or parent-child APIs. Once mounted, the framework owns the structural lifetime until removal/unmount.

Detached nodes are ordinary client-owned objects again and must not be treated as live framework nodes.

Framework services must resolve live nodes through the authoritative tree rather than assuming a stale pointer remains registered.

## Service state and node lifetime

Framework services that index nodes must not outlive the corresponding live-node registration.

This applies to modality and scrolling:

```text
NodeTree live node
      ↓
modal / scroll service state
      ↓
node removal
      ↓
service state is cleaned or invalidated
```

A removed modal must not remain the active modal. A removed scroll container must not leave stale scroll state that can affect coordinate transforms or wheel routing.

## Structural mutations

`PanelNode::addChild()` and `removeChild()` are framework-managed structural operations.

They are responsible for ownership, registration, lifecycle consequences, traversal safety and layout consequences through `NodeTree`.

A component must not directly modify NodeTree storage.

## Deferred mutation

Structural mutation during an active framework phase is deferred so that the current traversal observes a stable tree.

```text
Measure / Arrange / Draw
        ↓
mutation requested
        ↓
mutation queue / mutation scope
        ↓
current traversal completes
        ↓
mutation becomes visible
        ↓
later framework work observes stable tree
```

This applies to mutations that would otherwise invalidate iterators, traversal state or parent/child relationships during an active phase.

## Mutation and invalidation

Structural operations already know their layout consequences. They should not require a separate public `treeStructureChanged()` call.

For component-owned non-structural state, the framework does not observe arbitrary fields. If the change affects derived layout state, the component/client reports it explicitly with `invalidateLayout()`.

Scroll state is derived from committed layout geometry. A content or viewport change is reconciled during framework synchronization rather than requiring the client to maintain a second geometry model.

## Batch/coalescing principle

The framework owns scheduling and coalescing after a notification is made.

A caller may perform several local mutations and then issue one semantic notification/invalidation at the appropriate boundary.

For layout:

```text
mutation A
mutation B
mutation C
    ↓
invalidateLayout(root/domain)
    ↓
framework coalesces work
```

The current layout queue deduplicates roots. Repeated invalidation does not produce duplicate root jobs.

## Invalidation timing

`invalidateLayout()` is not a synchronous command. It means that committed layout-derived state may now be stale and the framework should reconsider the affected root during a future layout phase.

It does not:

```text
run layout immediately
flush mutations
flush the framework
mutate NodeTree directly
```

## Re-invalidation

If invalidation happens while Measure/Arrange is already running, the current traversal is not recursively restarted. The new request is queued for a later framework-controlled pass.

## Detached nodes

An invalidation request for a detached/non-live node must not create framework work. `NodeTree` is the authoritative source for this validation.

## Resource lifetime

For text, source `TTF_Font*` is client-owned and non-owning from the framework perspective.

```text
client owns TTF_Font*
        ↓
framework consumes it
        ↓
framework owns derived TTF_TextEngine / TTF_Text / raster-font copies
```

The source font must outlive every text component that may use it. The current client lifecycle already destroys UI users before closing the source font.

No general ResourceManager is justified until concrete requirements such as shared ownership, unloading, replacement or hot reload appear.

## Rendering resources

Derived rendering resources belong to the internal rendering/text backend. Their lifetime is tied to the framework/backend object that owns them, not to individual draw calls.

Font generation tracking is a renderer cache-consistency mechanism. It does not replace layout invalidation when font metrics may have changed.

## Protected recovery files

`node_tree.cpp` and `input_system.cpp` are protected recovery files. They were manually restored/adjusted during recovery and should not be mechanically rewritten during routine cleanup. Small surrounding include/signature fixes are acceptable.
