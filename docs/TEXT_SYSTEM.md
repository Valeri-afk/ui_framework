# Text System

## Current architecture

The active text path is:

```text
Typography / text-bearing controls / TextInput
        ↓
    TextContent
      ↙     ↘
 TextLayout  TextRenderer
     ↓           ↓
font-backed    SDL_ttf
measurement   rasterization/draw
```

`TextContent` is an internal bridge owning text presentation state and the Measure/Arrange/Draw connection. Components do not expose `TextRenderer`, renderer caches, `TTF_Text`, or other backend state as part of the public text contract.

The important architectural distinction is:

```text
TextLayout
    = retained text measurement state + logical measurement/wrapping
      using SDL_ttf font metrics

TextContent
    = component-facing text presentation and geometry bridge

TextRenderer
    = physical rasterization + SDL renderer integration
```

`TextLayout` is a logical measurement layer, but it is not a backend-independent typography engine. Its current implementation obtains font metrics through SDL_ttf. This is intentional for the current framework.

## Typography

`Typography` is the public standalone text component. There is no separate Heading / Paragraph component hierarchy.

`TypographyVariant` currently defines:

```text
INHERIT
H1–H6
SUBTITLE1/2
BODY1/2
BUTTON
CAPTION
OVERLINE
```

Variants are typography policy/semantic metadata, not independent layout algorithms.

## Text-bearing controls

`Button`, `MenuItem`, `TabItem` and `TextInput` use internal text state/`TextContent` rather than creating a retained `Typography` child merely to display their text.

## TextLayout architecture

`TextLayout` retains:

```text
text
source TTF_Font* (non-owning)
logical font size
logical line height
wrap mode
```

Its measurement contract accepts an available logical width and returns the desired logical size through the layout pipeline. `TextInput` additionally uses `TextContent` geometry queries for caret positioning and pointer-to-text-position mapping.

The implementation may use a temporary font copy when requested logical font metrics differ from the source font. That copy is local to measurement/render preparation and does not transfer ownership of the client source font.

### Why TextLayout depends on SDL_ttf

The current implementation intentionally uses SDL_ttf for font metrics:

```text
TextLayout   → SDL_ttf metrics
TextRenderer → SDL_ttf rendering
```

This still keeps logical measurement/wrapping separate from rasterization. A backend-independent typography layer should only be introduced for a concrete reusable requirement.

## TextLayoutResult

The layout result is intentionally small and currently contains the metadata required by the implementation, including desired size and line metrics/wrapping information. It is not an editor-state object and does not expose renderer internals.

## TextRenderer

`TextRenderer` is internal/backend-oriented. It owns physical/rendering concerns such as SDL_ttf text objects, derived raster fonts, renderer integration and renderer-state handling.

It does not own component editing state or the framework Measure/Arrange lifecycle.

## Measure → Arrange → Draw

Text participates in the framework lifecycle:

```text
Measure
   ↓
desired content size
   ↓
parent allocation
   ↓
Arrange
   ↓
text geometry for final content size
   ↓
Draw
```

For wrapping, the final arranged width is used by `TextContent` to establish the rendered layout corresponding to the committed allocation.

## Logical → physical rendering

The client uses a logical UI space with SDL logical presentation. Text sizes are logical values; the rendering layer derives the appropriate physical raster representation before SDL_ttf drawing.

## Font ownership

The source `TTF_Font*` is client-owned and non-owning from the framework perspective. The source font must outlive every text user.

Derived renderer resources are owned by the internal text renderer. There is no framework-wide font/resource manager contract.

## Font mutation

SDL_ttf font-generation tracking is a renderer cache-consistency mechanism. It does not replace layout invalidation. If a font change can alter metrics, wrapping or desired size, the affected owner must explicitly invalidate layout.

## Wrapping

The current supported policies are:

```text
WRAP
NO_WRAP
```

Truncation/ellipsis and richer line-breaking policies are deferred.

## Text input relationship

Editable text is implemented separately in `TEXT_INPUT_SYSTEM.md` as the public `TextInput` component. The editing state is not part of `Node` or `TextLayout`.

```text
TextInput
   ├── TextEditState      committed text/caret/selection
   ├── private IME state
   └── TextContent → TextLayout / TextRenderer
```

This preserves the boundary between text presentation/measurement and editing semantics.

## Non-goals

```text
public TextRenderer API
framework-wide font ResourceManager
rich text spans
ellipsis
advanced typography theme inheritance
backend-independent typography abstraction without a concrete requirement
```
