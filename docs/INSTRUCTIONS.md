# Framework Development Instructions

## 1. Source of Truth

Current source code is the authoritative source of truth.

Documentation describes the current or deliberately planned understanding of the codebase, but must never override actual implementation.

If documentation conflicts with source code:

> source code wins.

## 2. Repository Analysis

Before modifying code or architecture documentation:

1. Inspect the repository structure.
2. Read the relevant focused documentation.
3. Read `docs/FRAMEWORK_SCOPE.md` to understand purpose, responsibilities and scope boundaries.
4. Read `ARCHITECTURE.md` when the task depends on global architecture, ownership or subsystem relationships.
5. Analyze the actual source code.
6. Reconstruct the current behavior and contracts.
7. Identify known limitations and unfinished areas.

Do not treat planned behavior as existing implementation.

## 3. Before Architectural Changes

For every significant change determine:

- current behavior;
- current responsibility;
- current dependencies;
- invariants;
- public contract;
- affected modules;
- unaffected modules;
- target behavior;
- minimal required change.

Prefer the smallest change that establishes a clear and stable contract.

## 4. Refactoring Rules

- No giant refactors.
- Modify one fundamental architectural layer at a time.
- Do not introduce abstractions prematurely.
- Do not change unrelated modules.
- Preserve the existing public API unless there is a concrete reason to change it.
- Do not solve future problems prematurely.
- Prefer explicit runtime contracts over implicit conventions.

## 5. Implementation Safety

Preserve or explicitly redefine, rather than accidentally break:

- ownership;
- lifetime;
- NodeId/live-node invariants;
- mutation safety;
- traversal correctness;
- node attachment semantics;
- event behavior;
- layout invalidation and scheduling;
- Measure/Arrange ordering;
- rendering behavior;
- modal and input routing boundaries.

If ownership, mutation, lifecycle or another fundamental runtime semantic changes intentionally, update the corresponding focused documentation before or together with the implementation.

## 6. Testing / Verification

For significant changes consider:

- normal case;
- nested subtree;
- mutation during callback;
- deletion/removal of the current node;
- deferred mutation during protected traversal;
- layout invalidation followed by the next layout pass;
- event registration and dispatch;
- modal filtering and input routing;
- rendering after geometry changes;
- interaction with neighboring modules.

When a local build/test environment is not available, scenario analysis may still be performed, but unverified behavior must be explicitly identified as such.

## 7. Documentation

Focused documentation describes stable contracts and deliberate future boundaries. It must not become a historical log of implementation phases.

Update documentation when:

- an architectural/runtime contract changes;
- ownership or lifetime semantics change;
- public subsystem behavior changes;
- a significant invariant is introduced or removed;
- a capability is deliberately deferred or made part of the framework scope;
- existing documentation is found to contradict the source or another focused document.

The documentation roles are:

```text
FRAMEWORK_SCOPE.md
    framework purpose, boundaries and capability criteria

ARCHITECTURE.md
    global architecture and subsystem relationships

LAYOUT_SYSTEM.md
    Measure / Arrange / geometry / invalidation

LIFETIME_AND_MUTATIONS.md
    ownership, identity, lifecycle and structural mutation

DEFERRED_OPERATIONS.md
    queued structural/runtime operations and execution boundaries

COMPONENT_DESIGN.md
    Node/component extension and composition rules

INPUT_SYSTEM.md
    input routing, hit testing, focus and pointer interaction

EVENT_DISPATCHING.md
    event types, registration and dispatch semantics

RENDERING_SYSTEM.md
    rendering traversal, logical presentation and renderer boundaries

MODALITY_SYSTEM.md
    modal state, filtering and modal routing

SCROLL_SYSTEM.md
    scrolling infrastructure and coordinate behavior

TEXT_SYSTEM.md
    text content, layout and rendering

TEXT_INPUT_SYSTEM.md
    editable text/input control

README.md
    documentation entry point and current high-level state
```

The source code remains authoritative for current behavior.