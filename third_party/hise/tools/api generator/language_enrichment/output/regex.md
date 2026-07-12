---
title: Regex
description: "Standard regular expressions for pattern matching across HISE APIs and UI features"

guidance:
  summary: >
    Reference for regular expression usage in HISE. All regex consumers use
    standard ECMAScript regular expressions - fully standard compliant with
    no HISE-specific modifications. This page lists every API method and UI
    feature that accepts regex patterns, with practical examples for common
    use cases like component filtering, sound selection, and text matching.
  concepts:
    - regex
    - regular expressions
    - pattern matching
    - Content.getAllComponents
    - Sampler.selectSounds
    - Engine.getRegexMatches
    - Engine.matchesRegex
    - String.match
  prerequisites:
    - hisescript
  complexity: beginner
---

HISE uses standard ECMAScript regular expressions for all pattern matching. The syntax is fully standard compliant — there are no HISE-specific modifications or limitations to the regex engine itself.

In practice, regex appears in three contexts:

- **Component and module lookups** — `Content.getAllComponents`, `Synth.getAllModulators`, and similar API methods use regex to select references by name pattern.
- **Sample selection** — `Sampler.selectSounds` matches filenames with regex plus HISE-specific prefix operators for building complex selections.
- **String processing** — `Engine.getRegexMatches`, `Engine.matchesRegex`, and `String.match` provide general-purpose pattern matching and capture group extraction.

**See also:** [Usage in HISE](#usage-in-hise) -- every API method and UI feature that accepts regex patterns


## The Language

HISE's regex flavour matches JavaScript's ECMAScript syntax. For full syntax documentation, refer to the [MDN Regular Expressions guide](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Guide/Regular_expressions) — everything documented there applies in HISE.

### Quick Reference

| Feature | Syntax | Example |
| --- | --- | --- |
| Character classes | `[a-z]`, `\d`, `\w`, `\s` | `[A-G]` matches note names |
| Quantifiers | `*`, `+`, `?`, `{n,m}` | `\d{1,3}` matches 1-3 digits |
| Anchors | `^`, `$` | `^Filter` matches names starting with "Filter" |
| Groups and captures | `(...)` | `(\d+)\.(\d+)` captures version parts |
| Alternation | `\|` | `wav\|aif` matches either extension |
| Lookahead | `(?=...)`, `(?!...)` | `Knob(?!Master)` matches Knob but not KnobMaster |

> [!Warning:Double-escape backslashes in HiseScript] In HiseScript strings, backslashes must be doubled: write `"\\d+"` to match digits, not `"\d+"`. This applies to all regex-accepting API methods.

### Common Patterns

| Pattern | Matches | Use case |
| --- | --- | --- |
| `Knob[0-9]+` | `Knob1`, `Knob12`, `Knob100` | Component arrays by naming convention |
| `.*Button` | `PlayButton`, `StopButton` | All components ending with "Button" |
| `^Filter` | `FilterCutoff`, `FilterQ` | Components starting with "Filter" |
| `.*_RR[1-4]_.*` | Filenames with round-robin 1-4 | Sample selection by round-robin |
| `(\d+)` | Capture numeric parts | Extract numbers from strings |
| `.*\.wav` | Files ending in `.wav` | File type filtering |
| `[A-G]#?[0-9]` | `C3`, `F#4`, `Bb2` | Note name matching |


## Usage in HISE

Regex patterns are consumed by scripting API methods and by UI features in the HISE editor. Each method below takes a regex string and matches it against a domain-specific set of names.

### Select UI components by name

`Content.getAllComponents(regex)` returns an array of UI component references whose names match the pattern. This is the most commonly used regex consumer — essential for the modular code pattern where you grab batches of components by naming convention rather than referencing them individually.

```javascript
// All components matching a pattern
const var knobs = Content.getAllComponents("Knob[0-9]+");

// All components (fast path — skips regex evaluation)
const var all = Content.getAllComponents(".*");

// Buttons starting with "Filter"
const var filterBtns = Content.getAllComponents("Filter.*Button");
```

Inside a `ScriptDynamicContainer`, `ChildReference.getAllComponents(regex)` works the same way but is scoped to the children of that container's child reference:

```javascript
var buttons = childRef.getAllComponents("Button.*");
```

> [!Tip:Use `.*` for the all-components fast path] When you need every component, `Content.getAllComponents(".*")` is optimised to skip regex evaluation entirely. It's the idiomatic way to grab everything.

**See also:** $API.Content.getAllComponents$ -- full method documentation

### Select modules from the module tree

`Synth.getAllModulators(regex)` and `Synth.getAllEffects(regex)` return arrays of modulator or effect references whose IDs match the pattern. Use these to grab batches of modules by naming convention — the same pattern as component selection, but for the module tree.

```javascript
const var lfos = Synth.getAllModulators("LFO.*");
const var envelopes = Synth.getAllModulators(".*Envelope.*");
const var reverbs = Synth.getAllEffects("Reverb.*");
const var allFx = Synth.getAllEffects(".*");
```

**See also:** $API.Synth.getAllModulators$ -- full method documentation, $API.Synth.getAllEffects$ -- full method documentation

### Filter sampler sounds

`Sampler.selectSounds(regex)` selects sampler sounds whose filename matches the pattern. This is the only regex consumer in HISE that extends the standard syntax — it supports prefix operators for building complex selections incrementally.

```javascript
// Select all round-robin variants 1 and 2
Sampler.selectSounds(".*_RR[12]_.*");

// Add to current selection
Sampler.selectSounds("add:.*_C3_.*");

// Subtract from current selection
Sampler.selectSounds("sub:.*noise.*");

// AND operator — must match both patterns
Sampler.selectSounds(".*Piano.*&.*ff.*");
```

| Prefix | Behavior |
| --- | --- |
| *(none)* | Replace selection with matches |
| `add:` | Add matches to current selection |
| `sub:` | Remove matches from current selection |
| `&` (between patterns) | AND — sound must match all patterns |

> [!Warning:Prefix operators are pre-processed, not regex] The `add:`, `sub:`, and `&` operators are stripped before the regex engine sees the pattern. They are not part of the regex syntax itself — don't escape them or include them in capture groups.

**See also:** $API.Sampler.selectSounds$ -- full method documentation

### Match and extract from strings

Three methods provide general-purpose pattern matching on string values:

`Engine.getRegexMatches(string, regex)` returns an array of all matches including capture groups. The first element is the full match, followed by each capture group:

```javascript
var m = Engine.getRegexMatches("v1.2.3", "(\\d+)\\.(\\d+)\\.(\\d+)");
// m[0] = "1.2.3" (full match)
// m[1] = "1"     (first capture group)
// m[2] = "2"     (second capture group)
// m[3] = "3"     (third capture group)
```

`Engine.matchesRegex(string, regex)` returns `true` if the pattern is found anywhere in the string (partial match, not full-string match):

```javascript
if (Engine.matchesRegex(filename, "C[0-9]"))
    Console.print("Contains a C note");
```

`String.match(regex)` is called on a string value and returns an array of all matches:

```javascript
var results = "hello world 123 test 456".match("\\d+");
// ["123", "456"]
```

> [!Tip:Partial match, not full match] `Engine.matchesRegex` finds the pattern *anywhere* in the string. If you need a full-string match, anchor with `^` and `$`: `Engine.matchesRegex(name, "^Knob\\d+$")`.

**See also:** $API.Engine.getRegexMatches$ -- full method documentation, $API.Engine.matchesRegex$ -- full method documentation

### Regex in the HISE editor

The **Sampler Table** search field accepts regex patterns with the same syntax and prefix operators (`add:`, `sub:`, `&`) as `Sampler.selectSounds()`. Type a pattern to filter the sample list in real time.

The **Code Editor** Find dialog has a regex toggle. When enabled, the search term is interpreted as a regex pattern instead of plain text.


## Differences from Standard Regex

HISE uses standard ECMAScript regular expressions with no deviations. All ECMAScript regex features work as documented.

The only HISE-specific extension is the set of prefix operators in `Sampler.selectSounds` (`add:`, `sub:`, `&`), which are processed before the regex engine evaluates the pattern. These are not modifications to the regex syntax — they are a selection-management layer on top of standard regex.


**See also:** $API.Engine.getRegexMatches$ -- full match extraction, $API.Engine.matchesRegex$ -- boolean pattern test, $API.Content.getAllComponents$ -- component filtering, $LANG.hisescript$ -- the scripting language that uses regex patterns
