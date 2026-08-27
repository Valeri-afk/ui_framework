# Modality System

## Role

Modality is framework infrastructure, not a public `Modal` component.

The modality service is internal and is exposed to client code through `UIManager`.

## Responsibilities

The modality subsystem owns:

```text
modal registration / stack
active modal
modal-root input filtering
modal focus initialization and focus trap
backdrop state
backdrop rendering
Escape handling
outside-click handling
pointer-capture cancellation on open
viewport synchronization
modal cleanup
```

The client-facing surface is semantic:

```cpp
uiManager.showModal(node);
uiManager.showModal(node, BackdropClickBehavior::Close);
uiManager.showModal(node, options);

uiManager.closeModal();
uiManager.isModal(node);
uiManager.getActiveModal();
```

Backdrop configuration is exposed through the framework facade where required.

## Modal stack

Modality is stack-oriented.

```text
bottom modal
    ↓
...
    ↓
top / active modal
```

The top modal is always the active interactive modal. Lower modals remain mounted and continue normal framework updates, but they cannot receive restricted input while another modal is above them.

The same `Node` must not be registered twice as a modal.

When the active modal closes, the previous modal becomes active again. Closing/removing a modal must not leave a detached node registered as active modality state.

## Input filtering

Conceptually:

```text
SDL input
   ↓
UIManager / InputSystem
   ↓
active modal exists?
   ↓ yes
restrict target traversal to active modal subtree
   ↓
outside-click policy / modal handling
```

Only the active modal owns interaction. Its descendants may receive input normally.

Application content and lower modals do not receive pointer or keyboard input through ordinary dispatch while the active modal is present.

The modal does not need to be focusable itself. On open, focus is assigned to the modal when possible; otherwise the first focusable descendant is selected.

Focus traversal is trapped within the active modal subtree. `Tab` advances through focusable descendants and wraps at the boundary.

If focus is inside the modal subtree, `Escape` is offered to the focused component first. If it does not consume the event, the modal's Escape policy determines whether the modal closes.

## Outside clicks and backdrop

Backdrop and modality are separate concepts.

The backdrop is a presentation layer owned by the framework. It may be enabled or disabled independently of outside-click handling.

```text
showBackdrop = true / false

outsideClick = Consume / Close
```

An outside click means a pointer interaction outside the active modal bounds. It does not require a visible backdrop.

`BackdropClickBehavior` remains the public API name because backdrop interaction is a familiar modal concept; it does not imply that the backdrop is itself the modal's structural component.

When configured to close, the outside click initiates modal closing. When configured to consume, the event remains blocked and the modal stays open.

## Rendering and fade lifecycle

The backdrop is rendered by modality infrastructure rather than being a client-created child component.

Backdrop appearance is framework-configurable. Fade-in/fade-out timing belongs to the modality presentation lifecycle. Closing must not leave stale modal state registered while a visual transition is completing.

The modal stack remains a logical state machine independent of the visual backdrop.

## Pointer capture

Opening a modal establishes a new input boundary. Existing pointer capture/drag interaction outside that boundary must not leak through the new modal.

The modality transition therefore cancels incompatible pointer capture state when a modal opens. Scrolling or lower-modal updates do not implicitly reset capture on their own.

## Viewport

Modal and backdrop presentation follows the current framework viewport. Client code does not maintain a second modal viewport or manually resize a backdrop node.

Physical window resolution and logical presentation configuration remain renderer/client concerns; modality consumes the framework's current presentation geometry.

## Lifecycle and mutations

A modal is valid only while its node is live in `NodeTree`.

If a modal node is removed, modality state must be cleaned up so that no stale pointer remains registered. Nested modal state is resolved according to the remaining live stack.

Modal operations respect the framework's mutation/lifetime rules and must not make traversal unsafe.

## Public component decision

Do not introduce a public `Modal` component merely to represent the service. Ordinary `Node`/`PanelNode` hierarchy plus `UIManager` modality operations are sufficient for the current contract.

A dedicated component should only be introduced if real application use demonstrates a reusable semantic abstraction that cannot be expressed by the existing service and node composition.

## Regression coverage

The current modality regression suite covers:

```text
modal stack and active-input ownership
lower-modal updates while blocked
initial focus on modal / first focusable descendant
focus traversal and Tab wrapping
focus restoration after nested modal close
keyboard isolation between modal levels
Escape handling and closeOnEscape policy
outside-click close
outside-click consume
backdrop-independent outside click handling
pointer capture cancellation on open
nested modal activation and restoration
modal removal / stale-state cleanup
backdrop creation and fade lifecycle
```
