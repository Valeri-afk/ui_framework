# Text Input System

## Status

The current `TextInput` implementation is a **single-line, game-oriented editing control**. It is intentionally not a desktop text editor.

Implemented responsibilities include committed text, caret, selection, keyboard editing, mouse caret/drag selection, focus integration, SDL text input routing, basic IME composition state, and caret/selection presentation.

The current implementation does **not** own SDL window text-input lifecycle and does not expose window-specific IME APIs. Those remain optional until a concrete game/client requirement justifies them.

## Architectural boundary

```text
Node
  │
  └── focusability / normal Node lifecycle

InputSystem
  │
  ├── SDL keyboard input
  ├── SDL text input / IME events
  ├── focus routing
  ├── pointer routing / capture
  └── dispatch to focused Node

TextInput component
  │
  ├── committed text
  ├── caret
  ├── selection
  ├── editing commands
  ├── private composition / IME state
  └── text-input presentation policy

TextEditState
  │
  └── editable text/caret/selection state and mutation operations

TextContent
  │
  └── controlled bridge to text layout/rendering geometry

TextLayout
  │
  ├── measurement
  ├── wrapping
  └── focused single-line geometry queries used by TextInput

TextRenderer
  │
  └── text rasterization / SDL_ttf rendering
```

The central rule is that text editing remains a specialized component concern. `Node` does not gain generic editing state such as caret, selection or composition buffers merely to support text input.

## Input flow

Keyboard commands and actual text insertion are separate concepts:

```text
SDL keyboard events
        ↓
KeyDown / KeyUp
        ↓
editing commands

SDL text input
        ↓
TextInputEvent
        ↓
insert committed Unicode text

SDL text editing / IME
        ↓
TextEditingEvent
        ↓
update temporary private composition state
```

The framework does not reconstruct Unicode text from key codes. `InputSystem` translates SDL text-input events into framework events and routes them through the existing focused-node event path.

## Focus integration

There is one focus model. `InputSystem` owns actual focus transitions and keyboard routing.

A `TextInput` becomes active through the normal focus system. Losing focus clears temporary composition state and stops the control from consuming subsequent text events.

The framework does not introduce a second keyboard bus or a separate text-focus subsystem.

SDL text-input activation is intentionally **outside the current framework contract**. The window belongs to the client, and the current `TextInput` does not require `SDL_Window*` to be exposed through `UIManager`.

## Editing state

The implementation keeps three logically separate categories of state:

```text
Committed text
    actual stored string

Selection / caret
    caret position
    selection anchor/range

IME composition
    private temporary composition text
    private composition cursor/selection metadata
```

IME composition does not change committed text. A later `TextInputEvent` represents committed text and clears the temporary composition state.

Composition state is intentionally not part of the public `TextInput` API.

## Editing commands

The current game-oriented editing contract covers:

```text
Left / Right
Home / End
Backspace / Delete
select all
insert committed text
replace selection
Shift-based selection
mouse caret positioning
mouse drag selection
```

Clipboard, word-wise navigation, undo/redo and rich editor behavior are intentionally outside the current contract.

## TextLayout relationship

`TextLayout` remains a logical measurement/layout abstraction. It does not own editing state.

The current single-line input implementation uses a small set of focused geometry queries:

```text
Text position → caret X offset
pointer X      → text position
```

These queries are intentionally narrow. `TextLayoutResult` is not an editor-state object and does not expose renderer internals.

## Rendering relationship

Editing semantics remain outside `TextRenderer`:

```text
TextInput::draw()
    ├── selection presentation
    ├── TextContent → text rendering
    └── caret presentation
```

`TextRenderer` remains responsible for text rasterization and SDL_ttf/backend state. It does not own the editing buffer, caret, selection or IME state.

## Layout invalidation

Committed text mutations call the existing explicit layout invalidation path because text can change desired size and wrapping.

Render-only changes such as caret/selection presentation do not require Measure/Arrange invalidation.

This follows the framework rule that ordinary setters do not universally imply automatic layout invalidation; the operation that knows a layout-affecting change occurred requests recomputation.

## Mouse interaction

Mouse selection uses the normal framework pointer routing and capture semantics:

```text
pointer hit-test
     ↓
TextInput
     ↓
focus
     ↓
text-position query through TextContent/TextLayout
     ↓
caret / selection update
```

`TextInput` does not implement a second pointer-capture mechanism.

## Scope and non-goals

The current implementation intentionally targets common game UI scenarios such as names, chat fields, search/filter boxes, passwords and room codes.

Not currently required:

```text
framework-owned SDL_StartTextInput()/SDL_StopTextInput() lifecycle
SDL text-input-area / OS IME positioning API
clipboard system
undo/redo
word-wise navigation
multiline editor
text viewport/scrolling
rich text editing
backend-independent text engine
```

These should only be added when a concrete reusable game/client requirement appears.

## Validation

Regression coverage includes:

```text
committed text mutation
focused text input events
ignored text input without focus
keyboard editing
Shift selection
Ctrl+A selection
focus-loss behavior
SDL text-input routing
unfocused text-input isolation
SDL keyboard modifier routing
IME composition without mutating committed text
composition replacement
composition cleanup on focus loss
```
