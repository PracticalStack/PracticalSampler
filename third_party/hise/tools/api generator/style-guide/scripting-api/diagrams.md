# Diagram Creation Guidelines

Authoritative reference for the `diagram` field in the API enrichment pipeline. Covers when to add diagrams, how to write the plain-text description, and the full SVG rendering specification for Phase 4.

---

## Diagram Types

Four diagram types are supported. Each has a distinct visual layout and is suited to a specific kind of API behavior.

### `timing`

**Layout:** Horizontal swim lanes showing events dispatched across threads or time intervals.

**Use when:** A method involves a sync vs async dispatch choice, callback coalescing, repetition filtering, or different delivery threads depending on a flag.

**Example description:**
> When registered as `SyncNotification`, the callback fires immediately on the calling thread (which may be the audio thread -- use `inline function`). When registered as `AsyncNotification`, callbacks are coalesced and delivered on the UI thread at the next timer interval. Repeated values within one timer cycle are filtered to a single callback.

**Visual elements:** Thread lanes (audio thread, UI thread), event arrows crossing lanes, coalescing/filtering annotations, timeline direction left-to-right.

### `topology`

**Layout:** Three-column layout showing sources on the left, a central hub/object, and targets on the right. Fan-in/fan-out connections.

**Use when:** A method establishes a multi-object data flow, routing pattern, or connects multiple sources to multiple listeners.

**Example description:**
> A Broadcaster receives events from up to 11 source types (via `attachToComponentProperties`, `attachToComponentValue`, `attachToModuleParameter`, etc.) and distributes them to listeners registered with `addListener`, `addComponentPropertyListener`, `addComponentRefreshListener`, etc. Each source type defines its own argument signature.

**Visual elements:** Source boxes on left, hub box center, target boxes on right, connection lines with directional arrows, labels on connections for argument signatures.

### `sequence`

**Layout:** Vertical numbered timeline with steps flowing top to bottom. Phase boundaries separate init-time from runtime operations.

**Use when:** A method is part of a mandatory call sequence with ordering constraints, init-time restrictions, or a setup-then-use pattern.

**Example description:**
> Table mode setup requires three calls during `onInit` in order: (1) `setTableMode()` to activate, (2) `setTableColumns()` to define columns, (3) `setTableRowData()` to populate initial data. After init, only `setTableRowData()` can be called to update content. Calling setup methods after init has no effect.

**Visual elements:** Numbered step boxes, phase boundary line (labeled "onInit" / "runtime"), error annotations for out-of-order calls, optional branching for alternate paths.

### `state`

**Layout:** Boxes for states with labeled arrows for transitions between them.

**Use when:** A method switches the object between distinct behavioral modes with different API surfaces, or the object has a lifecycle with defined transitions.

**Example description:**
> The transport can be in one of three states: Idle, Playing, or Recording. `play()` transitions from Idle to Playing, `record()` transitions from Playing to Recording, and `stop()` returns any state to Idle. Recording is only reachable from Playing -- calling `record()` while Idle has no effect.

**Visual elements:** State boxes (rounded rectangles), transition arrows with method-name labels, entry/exit annotations, unreachable transitions marked with dashed lines or red.

---

## When to Add a Diagram

Writing a diagram description in Phase 1 is cheap -- just a type tag and 2-3 sentences of plain text. **Phase 4 decides which diagrams get rendered as SVGs and which get cut** in favor of prose or tables. Phase 1 agents should write diagram descriptions generously.

### Diagram type suggestions

These patterns are good candidates for diagrams, but the list is not exhaustive -- any behavior that benefits from a visual explanation is fair game:

- **Sync vs async dispatch**, callback coalescing, threading differences (`timing`)
- **Mandatory call ordering**, init-time restrictions, multi-step workflows (`sequence`)
- **Multi-object data flow**, fan-in/fan-out, routing, hub-and-spoke (`topology`)
- **Distinct behavioral modes**, lifecycle transitions, mode switches (`state`)

A class or method can have multiple diagram descriptions. There is no budget or limit.

### Class-Level vs Method-Level

- **Class-level diagrams** (in `Readme.md`) are appropriate for multi-method interactions: topology/architecture overviews, setup sequences involving multiple methods, state transitions triggered by different methods.
- **Method-level diagrams** are appropriate for behavior specific to a single method or a small group of related methods.

If a sequence diagram covers multiple methods (e.g. a three-step setup), attach it to the method that initiates the sequence (typically the first step). Other methods in the sequence use `diagramRef` to cross-reference it.

---

## JSON Schema

Diagrams exist at two levels in `api_reference.json`:

### Class-level diagrams

The `description.diagrams` field is an **array** of diagram objects. A class can have zero, one, or multiple diagrams. Each has a unique `id` used for cross-referencing from methods.

```json
"description": {
  "diagrams": [
    {
      "id": "table-setup",
      "brief": "Table Mode Setup Sequence",
      "type": "sequence",
      "description": "Table mode setup requires four calls...",
      "svg": "diagrams/ScriptedViewport/sequence_table-setup.svg"
    }
  ]
}
```

### Method-level diagrams (owned)

A method can have a single `diagram` object for behavior specific to that method. No `id` needed -- it is scoped to the method.

```json
"methods": {
  "registerCallback": {
    "diagram": {
      "brief": "Sync vs Async Dispatch",
      "type": "timing",
      "description": "SyncNotification fires immediately on the calling thread...",
      "svg": "diagrams/GlobalCable/timing_registerCallback.svg"
    }
  }
}
```

### Method-level diagram reference

When a method participates in a class-level diagram (e.g. one step in a multi-method sequence), it uses `diagramRef` instead of its own `diagram`. The value is the `id` of the class-level diagram.

```json
"methods": {
  "setTableMode": {
    "diagramRef": "table-setup"
  },
  "setTableColumns": {
    "diagramRef": "table-setup"
  }
}
```

A method has `diagram`, `diagramRef`, or neither. Never both.

### SVG file naming

- Class-level: `phase4/auto/ClassName/{type}_{id}.svg` (e.g. `sequence_table-setup.svg`)
- Method-level: `phase4/auto/ClassName/{type}_{methodName}.svg` (e.g. `timing_registerCallback.svg`)
- Manual overrides: same names in `phase4/manual/ClassName/` (takes priority)

---

## Readme.md Format

Class-level diagrams are defined in the `## Diagrams` section of the Phase 1 Readme.md. Each diagram is an h3 sub-section whose heading is the `id`.

```markdown
## Diagrams

### table-setup
- **Brief:** Table Mode Setup Sequence
- **Type:** sequence
- **Description:** Table mode setup requires four calls during onInit in strict order...

### display-modes
- **Brief:** Viewport Display Modes
- **Type:** state
- **Description:** The component operates in three mutually exclusive modes...
```

Omit the `## Diagrams` section entirely if no class-level diagrams are needed.

### Method-level format

In `methods.md`, a method-owned diagram:

```markdown
**Diagram:**
- **Brief:** Sync vs Async Dispatch
- **Type:** timing
- **Description:** SyncNotification fires immediately...
```

A method referencing a class-level diagram:

```markdown
**DiagramRef:** table-setup
```

---

## Description Writing

The `description` field is a plain-text description that serves two purposes:

1. **LLM consumption** -- LLMs and the MCP server cannot render SVGs, so the description is their only access to the diagram's content.
2. **SVG generation input** -- Phase 4 uses the description (plus the method/class context) to generate the SVG.

### Fields

- **`id`** (class-level only): Short kebab-case identifier. Used for `diagramRef` cross-references and SVG filenames.
- **`brief`** (required): 3-8 word human-readable label. Used for alt text on docs.hise.dev, section heading anchors, and LLM summaries. Example: "Table Mode Setup Sequence".
- **`type`**: One of `timing`, `topology`, `sequence`, `state`.
- **`description`**: 2-5 sentences (see rules below).
- **`svg`** (added by merge): Relative path to the SVG file.

### Description rules

- **2-5 sentences.** Enough to convey the key insight, not a full specification.
- **Cover the key insight** the diagram conveys -- the "aha" that a reader gets from the visual.
- **Name all states, modes, or paths** explicitly. Don't say "there are several modes" -- list them.
- **Mention threading** if relevant (which thread callbacks fire on, whether operations block).
- **Use concrete names** -- method names, property names, constant names. Not "the setup method" but "`setTableMode()`".
- **ASCII only** -- no em-dashes, curly quotes, or special characters.

---

## SVG Rendering Conventions

These conventions apply to Phase 4 auto-generated SVGs. Hand-crafted SVGs in `phase4/manual/` may deviate where design judgment dictates, but should stay visually consistent with the auto-generated style.

All colors, fonts, and spacing are derived from the HISE IDE style guide (`guidelines/style/cpp-ui.md`) to ensure diagrams feel native to the HISE environment.

### Rendering Philosophy

Diagrams and prose are complementary - they serve different modalities. The diagram shows the *shape* of the concept (connections, topology, flow, timeline). The surrounding prose explains the details. This means:

- **Visual clarity over textual completeness.** Do not try to embed all context into the diagram. A diagram that tries to be a standalone explanation becomes cluttered and defeats the purpose. Use a representative subset of items rather than exhaustive lists. For example, a topology diagram with 14 source types should show 4-6 representative sources and a "+ N more" indicator, not all 14.
- **No standalone explanatory text in diagrams.** Avoid bottom annotations like "All connections are visible in the XYZ panel" or internal labels like "change detection + priority sorting" that describe behaviour rather than structure. These belong in the prose that follows the diagram.
- **The diagram description field is input, not a rendering spec.** The Phase 1 `description` field provides rich context for LLM consumers and for the rendering agent to understand the concept. The SVG should distil this to the visual essence - do not replicate the description text verbatim in the diagram.

### Canvas

- **Background:** Transparent (no background rect or fill on the SVG element). Diagrams are rendered on the dark page background (`#0d1117`) in the preview HTML and on docs.hise.dev.
- **Panel fills:** `#161b22` (the page's secondary dark colour - for swim lanes, state boxes, step boxes, etc.)
- **Borders:** `rgba(0,0,0,0.5)` (matches `Colours::black.withAlpha(0.5f)`)
- **Corner radius:** `rx="6"` for all rounded rectangles
- **Dimensions:** 700-1000px wide, 400-660px tall. Choose dimensions that fit the content without excessive whitespace.
- **Padding:** 8px standard margin inside panels, 12px canvas edge margin

### Typography

- **Label font:** `Lato, sans-serif` at 14px (matches `GLOBAL_FONT()`)
- **Bold labels:** `Lato, sans-serif` at 14px, `font-weight: bold` (matches `GLOBAL_BOLD_FONT()`)
- **Code font:** `Consolas, 'Courier New', monospace` at 13px -- for method names, property names, and code snippets inside diagram boxes
- **Title font:** `Lato, sans-serif` at 16px bold
- **Primary text:** `#CCCCCC` (matches `Colours::white.withAlpha(0.8f)`)
- **Dimmed text:** `#808080` (matches `Colours::white.withAlpha(0.5f)`)
- **Use SVG `<text>` elements** -- not glyph paths from design tools (glyph paths are not searchable or accessible)

### Color Palette

Colors are derived from HISE's semantic color macros where possible. Two additional diagram-specific colors (purple, teal) are borrowed from HISE's module chain colors for source/target semantics.

| Color | Hex | HISE Source | Meaning |
|-------|-----|-------------|---------|
| Accent green | `#90FFB1` | `SIGNAL_COLOUR` | Accent, active state, highlight |
| Success green | `#4E8E35` | `HISE_OK_COLOUR` | Sync/safe operations, audio thread |
| Warning orange | `#FFBA00` | `HISE_WARNING_COLOUR` | Warnings, caution annotations |
| Error red | `#BB3434` | `HISE_ERROR_COLOUR` | Errors, restrictions, invalid operations |
| Purple | `#7E60B1` | Pitch modulation chain color | Source events, input triggers, "from" side |
| Teal | `#3F6E6E` | FX chain color | Async delivery, output, "to" side, UI thread |

- **Legend rules:**
  - Include a legend only when color or shape coding is not self-evident from inline labels, zone labels, or context.
  - If every colored/shaped element already has an inline text label that explains its meaning (e.g. "onInit" next to a green zone, "runtime" next to a teal zone), a legend is redundant -- omit it.
  - When a legend is needed, keep it to items where the color/shape distinction would be ambiguous without explanation. Do not include entries that describe rendering techniques ("Phase zone", "Inline code") or repeat information already labeled inline ("Phase boundary" when the boundary line itself is labeled).
  - Place the legend along the bottom edge, single row, minimal height.
- **Do not use color as the sole indicator** -- always pair color with text labels or shape differences for accessibility.

### Connectors and Arrows

- **Stroke width:** 1px for standard connectors, 2px for emphasis (e.g. the primary data flow path)
- **Arrow markers:** Define in `<defs>` using `<marker>` elements. Arrowhead size ~8x6px.
- **Dashed lines:** Use `stroke-dasharray="4,4"` for optional/conditional paths or unreachable transitions.
- **Orthogonal routing:** All connection paths must use right-angle bends (horizontal then vertical, or vertical then horizontal). Do not use diagonal lines between boxes at different vertical positions. Use SVG `<path>` elements with `L` commands for the bends, e.g. `d="M 222 85 L 260 85 L 260 200 L 310 200"`. This gives diagrams a clean, structured appearance. Bezier curves are acceptable only when paths must cross.
- **Stagger bend points** when multiple parallel connections share a corridor. Offset each path's bend x-coordinate by ~10px to prevent overlap (e.g. 256, 266, 276 for three parallel paths).

---

## Visual Vocabulary

The rendering spec above defines the visual *primitives* -- colors, fonts, boxes, arrows. This section defines higher-level *patterns* built from those primitives. A well-rendered diagram should communicate relationships and constraints visually: the reader should grasp the key insight from the shape and color of the diagram before reading any text labels. When in doubt, add visual richness -- a diagram that merely stacks labeled boxes adds little over prose.

### Shared Techniques

These techniques apply across all diagram types. Use them whenever the content calls for it.

#### Zone background tints

Faint colored rectangles behind logical regions of the diagram (e.g. onInit phase, runtime phase, thread lanes, source/target columns). Creates visual grouping without explicit boundary lines.

- Fill the zone rect with the region's accent color at `fill-opacity="0.04"` to `0.06`
- Add a matching border at `stroke-opacity="0.2"`, 1px
- Zone rects sit behind all other elements in draw order

#### Inline code blocks

Show the actual shape of structured parameters, object literals, enum value sets, or callback signatures directly inside the diagram. This communicates "what the data looks like" more effectively than a prose description.

- Render as a smaller sub-rect (`fill="#0d1117"`, `stroke="#444444"`) positioned below or inside the parent step/state box
- Use the code font at 9-10px for compact inline code
- Keep to 1-2 lines; if longer, this belongs in the API docs, not the diagram

#### Optional elements

Steps, connections, or transitions that are not required should be visually distinct from required ones.

- Use `stroke-dasharray="4,3"` for the element's border
- Use dimmed text color (`#808080`) instead of the accent color for labels
- Add a small `"optional"` label in dimmed text at the right edge of the element
- Always pair the dashed style with the text label (do not rely on dashes alone)

#### Error/constraint annotations (only when additive)

Error annotations should only appear when they convey constraints **not already encoded in the diagram's structure**. Before adding an error section, ask: does the existing visual already communicate this?

- A **phase boundary** already says "these methods are init-only" -- do not restate this as error annotations
- A **numbered timeline** already says "call in this order" -- do not restate ordering as errors
- A **dashed arrow** in a state diagram already says "this transition is invalid" -- do not add redundant text

**When to add error annotations:**

- A constraint that is non-obvious from the layout (e.g. "calling X in state Y corrupts data" in a state diagram)
- A threading hazard not visible in the swim lanes (e.g. "deadlocks if called from audio thread callback")
- A parameter constraint that a topology connection line cannot convey

**Format when used:**

- Prefix each error line with a colored "X" marker in the error color
- Show the invalid call/transition on the left, the reason on the right
- Group all error annotations in a clearly bounded region (faint error-colored background)

### Layout Patterns by Type

#### Timing diagrams

**Core layout:**

- Horizontal swim lanes, one per thread (e.g. "Audio Thread", "UI Thread")
- Lane height: 60-80px
- Events are circles or short vertical bars on the lane
- Cross-lane arrows show dispatch from one thread to another
- Time flows left to right

**Visual techniques:**

- **Coalescing brackets:** When multiple events collapse to one, draw a bracket or brace grouping the input events with a single output arrow. This is the key visual insight for async coalescing.
- **Filtering marks:** When repeated values are dropped/filtered, mark them with a small error-colored X on the lane to show "this event was swallowed."
- **Thread-crossing emphasis:** Arrows that cross between swim lanes should use 2px stroke to stand out from within-lane arrows.

> *Note: Timing visual techniques are initial guidance. To be refined after testing with real timing diagrams (e.g. GlobalCable.registerCallback).*

#### Topology diagrams

**Core layout:**

- Three columns: sources (left), hub (center), targets (right)
- Column width: ~250px each for a 1000px canvas
- Source/target boxes: 180x30px minimum, stacked vertically with 8px gap
- Hub box: larger, centered, with a distinct border (2px stroke)
- Connection lines: source-to-hub and hub-to-target, with arrowheads

**Visual techniques:**

- **Connection labels:** A single label per connection bundle (e.g. `attachTo*()` for all source connections) is preferable to individual labels on each line. Place labels with a background rect (see Text Readability below) at the corridor between the source/target column and the hub. Use at least 11px font size.
- **Representative subsets:** When a class has many source or target types, show 4-6 representative items and a dimmed "+ N more" indicator below the last box. List the full set in the surrounding prose. This keeps the diagram scannable.
- **Category grouping:** If sources or targets fall into natural subgroups, add subtle horizontal separators (1px, dimmed color) between groups and label each group.

#### Sequence diagrams

**Core layout:**

- **Timeline spine:** A continuous 2px vertical line running from the first step to the last, creating a visual "flow" that connects the sequence. Without the spine, steps appear as disconnected boxes stacked vertically.
- **Numbered circles on spine:** Each step gets a circle (`r="14"`, `fill="#0d1117"`, 2px accent-coloured stroke) centred on the spine, with the step number in bold inside. Step description boxes extend to the right of the spine (left edge at ~spine + 30px).
- Step boxes: ~40-50px tall, extending from the spine area to the right canvas edge minus padding
- Phase boundary: horizontal dashed line spanning full width, labeled (e.g. "onInit" / "runtime")
- **Spine termination:** The timeline spine should end at the last step circle or the phase boundary -- do not extend it into non-timeline regions (legends, annotations). A clean endpoint reinforces that the sequence is complete.

**Visual techniques:**

- **Phase zone backgrounds:** Behind each phase region (e.g. the onInit steps, the runtime steps), place a faint zone tint rect (see Shared Techniques above). Color each zone to match its phase label color (success green for onInit, teal for runtime). This visually groups the steps belonging to each phase.
- **Repeat/loop arrows:** When a step "can be called repeatedly" or "at any time," add a curved return arrow on the right edge of the step box. Use a quadratic bezier (`<path>` with Q control point) that arcs out to the right and loops back to the top of the same box. Color it to match the step's phase. Add a small filled triangle arrowhead at the return point.
- **Optional steps:** Steps that are not required in the sequence use dashed borders and dimmed text instead of the accent color. The numbered circle on the spine also gets a dashed stroke. Add an "optional" label (see Shared Techniques).
- **Inline parameter shapes:** When a step accepts a structured parameter (JSON object, array of objects, enum set), show a compact inline code block below the step's description line (see Shared Techniques). For example, showing `{ "CallbackOnSliderDrag": true, "MultiColumnMode": false }` inside the `setTableMode` step box communicates the parameter shape at a glance.
- **Optional branching:** When the sequence has alternate paths, fork the timeline with a decision diamond or split arrows. Rejoin at the convergence point.
- **Legend:** Sequence diagrams with labeled phase zones and a labeled phase boundary are typically self-documenting -- omit the legend. A legend is only needed if the diagram introduces a color or shape distinction not covered by inline labels (e.g. an "optional" dashed-border step alongside required steps, where the dashed style needs explanation).

#### State diagrams

**Core layout:**

- State boxes: rounded rectangles, 120x50px minimum
- Arrange in a logical layout (often circular or left-to-right flow)
- Transition arrows: labeled with the method/event that triggers the transition

**Visual techniques:**

- **Entry marker:** Small filled circle (r=4, accent color) with an arrow pointing to the initial state. This is a standard UML convention that immediately identifies the starting state.
- **Self-transition loops:** For idempotent operations or events that keep the object in the same state, draw a curved arrow that leaves and returns to the same state box (arc above or below the box).
- **Unreachable/invalid transitions:** Dashed red arrows with explanation text. Use the error color and dimmed text for the reason.

> *Note: State visual techniques are initial guidance. To be refined after testing with real state diagrams.*

---

## Text Readability and Overlap Check

After rendering a diagram, perform a final review pass for text readability issues. These are common and easy to miss when writing SVG by hand.

### Labels near connectors

Any text label positioned near connector lines or arrows must have a **background rect** behind it for readability. Without this, the text blends into the arrows and becomes hard to read against the dark transparent canvas.

**Implementation:**

```xml
<!-- Background rect: slightly larger than the text, semi-transparent -->
<rect x="{textX - padding}" y="{textY - fontSize}" width="{textWidth + 2*padding}" height="{fontSize + 6}" fill="#161b22" fill-opacity="0.85" rx="3"/>
<text x="{textX}" y="{textY}" ...>label</text>
```

- Use `fill="#161b22"` with `fill-opacity="0.85"` for the background
- The rect must be placed *before* the text element in SVG draw order
- Minimum font size for labels near connectors: 11px (9px is too small)

### Checklist

After completing the SVG, verify:

- [ ] No text overlaps with connector lines without a background rect
- [ ] All labels are at least 11px font size
- [ ] All connection paths use orthogonal routing (no diagonal lines)
- [ ] Zone labels ("SOURCES", "TARGETS", thread names) are outside or at the top edge of their zones, not overlapping content
- [ ] The diagram is visually scannable at a glance before reading any text

---

## Manual Override

To lock a diagram (prevent Phase 4 auto from regenerating it):

1. Copy the SVG from `phase4/auto/ClassName/` to `phase4/manual/ClassName/`
2. Edit the manual copy as needed
3. The merge process and Phase 4 agent both skip diagrams that already exist in `phase4/manual/`

Hand-crafted SVGs from design tools (Affinity Designer, Inkscape, etc.) should be placed directly in `phase4/manual/`. These are never overwritten.
