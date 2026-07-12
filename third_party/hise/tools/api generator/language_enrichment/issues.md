# Language Enrichment — Issues

Bugs, implementation gaps, and improvement opportunities discovered during language reference authoring.

## CSS

### `text-transform: capitalize` parsed but not functional

**Source:** `hi_tools/simple_css/StyleSheet.cpp:1894-1895`
**Severity:** Minor (Low complexity fix)
**Description:** The CSS property `text-transform: capitalize` is parsed and accepted by the CSS engine, but the implementation returns the text unchanged. `uppercase` and `lowercase` work correctly. The branch exists but doesn't apply the transformation.

### `font` shorthand not implemented

**Source:** `hi_tools/simple_css/CssParser.cpp:2212`
**Severity:** Minor (Low-Medium complexity)
**Description:** The `font` shorthand property (e.g., `font: bold 16px Arial`) is detected by the parser with a `jassertfalse; // soon` comment, but is not actually processed. Should parse into individual `font-family`, `font-size`, `font-weight` properties — similar to the existing `border` shorthand expansion logic.

### `border-style` dashed/dotted not rendered

**Source:** `hi_tools/simple_css/Renderer.cpp:825, 1040`
**Severity:** Minor (Low complexity fix)
**Description:** The CSS parser correctly stores `border-style` values including `"dashed"` and `"dotted"`, but the renderer ignores the value — both `g.strokePath()` call sites use plain `PathStrokeType(borderSize)` without checking the style. JUCE natively supports dash patterns via `PathStrokeType::createDashedStroke(Path&, const Path&, const float*, int)`. Fix requires branching on the stored `border-style` value at the two stroke call sites (~10 lines of code). The `NonUniformBorderData::draw()` method in `HelperClasses.cpp:725` could optionally be extended for per-side dash support.

### Nested `calc()` expressions not supported

**Source:** `hi_tools/simple_css/CssParser.cpp:890-1159`
**Severity:** Minor (Medium complexity)
**Description:** The expression parser handles `calc()`, `min()`, `max()`, and `clamp()` at a single level, but does not accept nested `calc()` expressions like `calc(100% - calc(50px + 2em))`. The parser already handles the arithmetic; it needs to recognize `calc` as a recursive token within an expression.

### Multiple backgrounds not supported

**Source:** `hi_tools/simple_css/StyleSheet.cpp` (background resolution), `Renderer.cpp` (background drawing)
**Severity:** Minor (Medium complexity)
**Description:** Standard CSS allows comma-separated background layers that render back-to-front. HISE currently supports only a single background-color or gradient per element. Implementation would require storing an array of background layers in `StyleSheet` and iterating in reverse during rendering. The parser already handles comma-separated values in other contexts (shadows).

### `transition-property` as separate declaration not supported

**Source:** `hi_tools/simple_css/CssParser.cpp` (transition parsing)
**Severity:** Minor (Low complexity)
**Description:** Transitions can only be declared via the `transition` shorthand (e.g., `transition: background-color 0.5s ease`). Separate `transition-property`, `transition-duration`, `transition-timing-function`, and `transition-delay` declarations are not recognized. The parser already handles the shorthand correctly; it needs to accept the individual properties and combine them.

## HiseScript

### Destructuring assignment

**Severity:** Minor (Medium complexity)
**Description:** JavaScript's destructuring syntax (`var {a, b} = obj`) is not supported. This would be particularly useful in graphics code for unpacking Rectangle objects and in API calls that return multi-property objects. Implementation would intercept the `{` character at the variable definition point and branch into a destructuring parser. Should also work with the native Rectangle object class. HiseScript's existing strict argument checking suggests that referencing a property name that does not exist on the source object should throw a script error rather than silently assigning `undefined` (diverging from JavaScript's lenient behaviour).

## SNEX

### Forward declaration order required in templated structs

**Severity:** Minor (Medium complexity)
**Description:** In `template <int NV>` structs (which covers most SNEX nodes), methods must be defined before they are called by other methods within the same struct. For example, a `prepare()` method cannot call `updateFilter()` if `updateFilter()` is defined later in the struct body. This works correctly in non-templated classes, where standard C++ lookup rules apply. The JIT compiler's template instantiation appears to resolve method references in a single forward pass rather than using two-pass lookup.
