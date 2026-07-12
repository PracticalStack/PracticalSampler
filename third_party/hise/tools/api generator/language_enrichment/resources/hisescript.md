---
title: The HiseScript Language
description: "Variables, types, functions, and language features — everything you need to write HiseScript"

guidance:
  summary: >
    Complete guide to the HiseScript language. Covers all five variable types
    (const var, var, reg, local, global) with realtime-safety implications,
    data types and type annotations, arrays and realtime-safe container
    alternatives (Buffer, MidiList, FixObjectArray, UnorderedStack), objects
    and the factory pattern, copy semantics and debugging tools (clone, trace,
    Script Watch Table), operators with short-circuit patterns, control flow,
    loops (for vs for...in performance), functions (inline vs regular with
    RT-safety reasoning, variable capturing), namespaces and scope rules,
    and the central modular code pattern (arrays of components, shared
    callbacks, indexOf). Assumes the reader has read Structure > Scripting
    Model and understands the interface script pattern, compilation model,
    callbacks, and module/UI references.
  concepts:
    - HiseScript
    - variable types
    - const var
    - var
    - reg
    - local
    - global
    - Globals
    - data types
    - type annotations
    - isDefined
    - arrays
    - Buffer
    - MidiList
    - FixObjectArray
    - FixObjectStack
    - UnorderedStack
    - objects
    - object factory
    - clone
    - trace
    - Script Watch Table
    - operators
    - short-circuit evaluation
    - control flow
    - ternary operator
    - switch
    - for loop
    - for...in
    - inline function
    - function
    - variable capturing
    - namespaces
    - include
    - modular code
    - arrays of components
    - indexOf
    - setControlCallback
  prerequisites:
    - scripting-model
    - module-tree
  complexity: intermediate
  architecture_counterpart: scripting-model
---

This guide covers the HiseScript language itself — syntax, types, functions, and the patterns you'll use every day. It assumes you've read the [Scripting Model](/v2/architecture/scripting-model), which explains *how scripting fits into HISE*: the interface script pattern, compilation model, callbacks, module references, and `setControlCallback`. This page is about *how to write the code* that goes inside those structures.

HiseScript looks like JavaScript — curly braces, dot notation, familiar operators — but it is its own language. No ES6 features, no closures, no prototype chain, and a set of variable types that exist specifically for realtime audio safety.


## Variables

HiseScript **requires** an explicit keyword when creating a variable. You cannot assign to an undeclared name — doing so produces a compile error. This prevents the notorious JavaScript bug where a typo silently creates a new variable:

```javascript
var myDatabaseEntry = "Hello";
myDataBaseEntry = "Hello again"; // Compile error — capital B is a typo
```

The only exception is the counter variable in a `for` loop, which can be used without a keyword.

### const var

A constant — its value cannot change after declaration. By convention, simple constants use `UPPER_CASE`; component and module references mirror the component's name.

```javascript
const var NUM_BUTTONS = 8;
const var filter = Synth.getEffect("PolyphonicFilter");
const var knobCutoff = Content.getComponent("knobCutoff");
```

`const var` is more than a safety measure. The compiler replaces every reference with its literal value, making it the most efficient variable type. **Use `const var` as your default** — for module references, UI handles, lookup tables, and any value that doesn't change.

### var

Mutable and accessible from any callback when declared in `onInit`. This is the **least efficient** type and has a critical limitation: `var` allocates memory at runtime, making it **not realtime-safe**. Never use `var` inside audio-thread callbacks (`onNoteOn`, `onNoteOff`, `onController`, `onTimer`).

```javascript
var currentPage = 0;        // UI state — scripting thread only
var selectedPreset = "";    // changes over time
```

Another quirk: `var` declared inside a `namespace` **leaks into the global scope**. Always use `reg`, `const var`, or `local` inside namespaces.

### reg

Register variables are stored in a pre-allocated memory block. Because the memory is reserved at compile time, reading and writing a `reg` never triggers allocation — making it **realtime-safe** and the correct choice for mutable state used in audio-thread callbacks.

```javascript
reg lastNote = -1;          // safe to read/write in onNoteOn
reg rrCounter = 0;          // round-robin counter
reg sustainDown = false;    // CC64 state
```

The limitation: **32 `reg` variables per scope** (the main script scope, and each namespace gets its own 32).

### local

Exists only within the enclosing `{}` scope. Cannot be declared in `onInit` (which has global scope). Inside an `inline function`, `local` variables are pre-allocated and **realtime-safe**.

```javascript
inline function processNote()
{
    local note = Message.getNoteNumber();
    local vel = Message.getVelocity();
    // note and vel don't exist outside this function
};
```

Use `local` for loop variables, temporary calculations, and anything that doesn't need to persist between calls.

### global

Enables communication between separate `ScriptProcessor` modules in the same project. A `global` variable is accessible from any script in the module tree.

```javascript
global myGlobal = 10;
// Equivalent to: Globals.myGlobal = 10;
```

Always use the `Globals.` prefix when reading globals — it makes cross-script dependencies immediately visible:

```javascript
Globals.currentArticulation = 2;  // set in one script
Console.print(Globals.currentArticulation);  // read in another
```

### Decision Framework

| Type | Lifetime | RT-Safe | Use for |
|------|----------|---------|---------|
| `const var` | Script | ✅ | Module refs, UI handles, lookup tables, anything immutable |
| `var` | Script | ❌ | UI state (page index, mode flags) — scripting thread only |
| `reg` | Script | ✅ | MIDI callback state (RR counters, last note, CC values) |
| `local` | Scope | ✅* | Temporaries, loop variables, function-internal state |
| `global` | Project | ❌ | Cross-script communication |

*\*RT-safe inside `inline function` only.*


## Data Types

HiseScript has five basic types:

| Type | Example | Notes |
|------|---------|-------|
| **Number** | `58`, `3.14` | No int/float distinction |
| **String** | `"hello"`, `'world'` | **Not RT-safe** — allocates memory |
| **Boolean** | `true`, `false` | Equivalent to `1` and `0` |
| **undefined** | `var x;` | No value assigned |
| **null** | `var x = null;` | Explicitly empty |

String operations (concatenation, `Console.print()`) allocate memory. Never use them in audio-thread callbacks in production. `Console.print()` is fine during development — remove it before export.

### isDefined()

Check whether a variable has a value:

```javascript
if (isDefined(v))
    Console.print(v);

if (!isDefined(v))
    Console.print("v has no value");
```

More readable than `v != undefined` and the idiomatic way to guard against undefined values. Note: `isDefined()`, `trace()`, and `clone()` are built into the language but don't appear in the API Browser or autocomplete.

### Type Annotations

Optional type constraints that catch bugs at compile time with zero runtime overhead:

```javascript
reg:int noteNumber = 60;
noteNumber = "text";             // Compile error

inline function:int getIndex()   // return type
{
    return 3;
}

inline function setGain(value:number)  // parameter type
{
    // value is guaranteed to be a number
}
```

Common type identifiers: `int`, `double`, `number` (either), `string`, `Array`, `JSON`, `object`.


## Arrays & Collections

Arrays store multiple values, accessed by zero-based index:

```javascript
var a = [1, 2, 3];
a[1] = 57;                   // replace second element
a.push(99);                  // append
Console.print(a.length);     // 4

const var b = ["text", 3.14, [7, 8, 9]];
Console.print(b[2][0]);      // 7 — nested access
```

**Realtime safety:** operations that change an array's size (`push()`, assigning beyond length) allocate memory and are **not RT-safe**. For audio-thread work, use pre-allocated arrays with a fixed size, or one of these purpose-built containers:

| Type | Use case |
|------|----------|
| **Buffer** | Fixed-size float collection — sample data, DSP, numeric processing |
| **MidiList** | 128-slot integer array — velocity maps, transposition tables, key switches |
| **FixObjectArray** | Fixed-size array of structured objects with a shared property layout |
| **FixObjectStack** | Same as FixObjectArray but with push/pop semantics |
| **UnorderedStack** | Pre-allocated stack of numbers — tracking active voices or note IDs |

Standard arrays are fine for initialization and UI code. Consult the API Browser for each container's full API.


## Objects

Objects store data as key-value pairs:

```javascript
const var o = { name: "Flute", velocity: 100, enabled: true };

Console.print(o.name);           // Flute — dot notation
Console.print(o["velocity"]);    // 100 — bracket notation

o.velocity = 64;                 // change a value
o.newKey = [1, 2, 3];           // add a key
o.nested = { sub: 12 };         // nest objects
```

### The Factory Pattern

HiseScript has no `new` operator and no prototype chain. To create configured objects, use factory functions:

```javascript
inline function createInstrument(name, velocity)
{
    local obj = {};
    obj.name = name;
    obj.velocity = velocity;
    obj.enabled = true;
    return obj;
}

const var flute = createInstrument("Flute", 100);
```

The most common factory pattern in practice wraps `Content.getComponent()`:

```javascript
inline function createPanel(name, x, y)
{
    local p = Content.getComponent(name);
    p.set("x", x);
    p.set("y", y);
    return p;
}
```

This is idiomatic — UI elements must be created upfront during initialization, so there's no need for dynamic object construction at runtime.


## Copying & Debugging

Arrays and objects are **passed by reference**. Assigning one to another does not create a copy:

```javascript
var a = ["dog", "cat", "fish"];
var b = a;
b[1] = "chicken";
Console.print(a[1]); // "chicken" — a was modified too!
```

### clone()

Creates an independent deep copy:

```javascript
var b = a.clone();
b[1] = "chicken";
Console.print(a[1]); // "cat" — a is unaffected
```

### trace()

Converts an array or object to a readable string showing every element:

```javascript
const var o = { colors: ["red", "green"], gain: -6 };
Console.print(trace(o));
```

Indispensable for debugging LAF parameters, API return values, and nested data structures.

### Script Watch Table

The toolbar's **Script Watch Table** lets you inspect variable contents live. Right-click an array or object to expand it into a tree view. For variables inside functions (which don't appear in the table), use `trace()`.


## Operators

### Arithmetic

`+`, `-`, `*`, `/`, `%` (modulus), `++`, `--`. Standard precedence — use parentheses to override.

### String Concatenation

`+` also joins strings, and converts numbers to strings when mixed:

```javascript
var s = "Value: " + 10;     // "Value: 10"
```

When HISE sees a string on either side of `+`, it treats the operation as concatenation. This matters when constructing component names dynamically — see [Writing Modular Code](#writing-modular-code) below.

### Comparison & Logical

`==`, `!=`, `>`, `<`, `>=`, `<=` — return `1` (true) or `0` (false).

`&&` (AND), `||` (OR), `!` (NOT). Any non-zero value is `true`. Both `&&` and `||` use **short-circuit evaluation**:

```javascript
var name = obj && obj.getName();  // null guard — only calls getName() if obj exists
var gain = userGain || -6;        // default value — uses -6 if userGain is 0/undefined
```


## Control Flow

### if / else

```javascript
if (a == 10)
{
    Console.print("ten");
    b = 20;
}
else if (a == 5)
    Console.print("five");   // single line — braces optional
else
    Console.print("other");
```

### Ternary Operator

Compact conditional, widely used in LAF functions:

```javascript
var colour = obj.value ? obj.bgColour : obj.itemColour1;
```

### switch

Cleaner than if/else chains for matching a single value:

```javascript
switch (articulationIndex)
{
    case 0: sampler.loadSampleMap("Sustain"); break;
    case 1: sampler.loadSampleMap("Staccato"); break;
    case 2: sampler.loadSampleMap("Tremolo"); break;
    default: break;
}
```


## Loops

### for

Standard counting loop. The counter doesn't need a keyword:

```javascript
for (i = 0; i < panels.length; i++)
    panels[i].set("visible", i == selectedIndex);
```

Use when you **need the index** — comparing positions, writing back to specific indices, or correlating two arrays.

### for...in

Iterates elements directly (arrays) or keys (objects). **Significantly faster** than index-based `for` — use as your default:

```javascript
// Arrays — iterates values
const var names = ["Alice", "Bob", "Charlie"];
for (name in names)
    Console.print(name);

// Objects — iterates keys
const var o = { a: 10, b: 20 };
for (k in o)
    Console.print(o[k]);
```

### while

```javascript
var count = 0;
while (count < 20)
{
    Console.print(count);
    count++;
}
```

Ensure the condition becomes false — otherwise the loop locks the application.

### break and continue

`break` exits the loop. `continue` skips to the next iteration:

```javascript
for (i = 0; i < values.length; i++)
{
    if (values[i] < 0) continue;     // skip negative
    if (values[i] == target) break;  // stop searching
    Console.print(values[i]);
}
```


## Functions

### inline function (default choice)

Always use `inline function` for named, reusable functions — especially anything called from audio-thread callbacks. Regular `function` allocates a scope object on the heap every call. `inline function` pre-allocates its scope at compile time, making it **realtime-safe**.

```javascript
inline function addition(a, b)
{
    local c = a + b;
    return c;
}
```

Type annotations work on parameters and return values:

```javascript
inline function:int getVelocityLayer(vel:number)
{
    if (vel < 64) return 0;
    return 1;
}
```

### Regular function (anonymous callbacks only)

Use plain `function` when passing an anonymous function to an API call that doesn't run on the audio thread — paint routines, dialog handlers, file operations:

```javascript
Panel1.setPaintRoutine(function(g)
{
    g.fillAll(0xFF333333);
});

Engine.showYesNoWindow("Confirm", "Sure?", function(ok)
{
    if (ok) Console.print("Confirmed");
});
```

**Rule:** named + reusable → `inline function`. Anonymous + non-RT → `function`.

### Variable Capturing

Inner functions cannot access outer function parameters. HiseScript solves this with explicit capture lists (borrowed from C++ lambdas):

```javascript
inline function deletePreset(presetName)
{
    Engine.showYesNoWindow("Confirm", "Delete?", function [presetName](ok)
    {
        if (ok)
            Console.print("Deleting: " + presetName); // Works!
    });
}
```

List multiple captures with commas: `function [a, b, c](ok)`. This comes up with any API call that takes a callback — dialogs, file operations, asynchronous tasks.

### Custom Callbacks

Assign a dedicated handler to a UI control (covered in depth in the [Scripting Model](/v2/architecture/scripting-model)):

```javascript
const var Knob1 = Content.getComponent("Knob1");

inline function onKnob1Control(component, value)
{
    Console.print(value);
}

Knob1.setControlCallback(onKnob1Control);
```

Right-click a component in the Component List → **Create custom callback definition** to generate the boilerplate.


## Namespaces & Code Organization

Namespaces divide code into scoped, labeled sections — the primary organizational tool as projects grow:

```javascript
namespace Filters
{
    const var lp = Synth.getEffect("LowPass");
    reg currentFreq = 1000;

    inline function setCutoff(freq)
    {
        currentFreq = freq;
        lp.setAttribute(lp.Frequency, freq);
    }
}

// Access from outside
Filters.setCutoff(2000);
```

This is the same dot-notation pattern as `Console.print()` or `Engine.getSampleRate()`. By convention, namespace names start with a capital letter.

### Scope Rules

- Namespaces **cannot be nested**
- Names must not collide with HISE classes (`Engine`, `Content`, etc.)
- `var` inside a namespace **leaks into global scope** — always use `reg`, `const var`, or `local`
- Each namespace gets its own **32 `reg` slots**, independent of the main scope

### External Files

In practice, each namespace lives in its own `.js` file. Select the block → right-click → **Move selection to external file**. HISE adds the `include()` automatically.

A well-organized project's `onInit` becomes a series of includes:

```javascript
include("App.js");
include("Paths.js");
include("LookAndFeel.js");
include("Header.js");
include("Settings.js");
```

Each file contains one namespace. For how this scales across project sizes, see [Script Organization](/v2/architecture/scripting-model#script-organization) in the Scripting Model.


## Writing Modular Code

This is the central practical pattern in HiseScript. Whenever you find yourself writing the same code for multiple controls with only a name changing, refactor into arrays and loops.

### The Problem

Three buttons with radio-group behavior. A naive approach: three separate callbacks with near-identical code. At ten buttons, it's unmanageable.

### Arrays of Components

Store references in an array, populated by a loop:

```javascript
const var NUM_BUTTONS = 3;
const var buttons = [];

for (i = 0; i < NUM_BUTTONS; i++)
    buttons[i] = Content.getComponent("Button" + (i + 1));
```

The parentheses around `(i + 1)` are essential. Without them, `+` sees the string `"Button"` on the left and treats everything as concatenation — producing `"Button01"` instead of `"Button1"`.

### Shared Callback

One function handles all buttons:

```javascript
const var Label1 = Content.getComponent("Label1");

inline function onButtonControl(component, value)
{
    for (i = 0; i < buttons.length; i++)
    {
        if (buttons[i] == component)
            continue;  // skip the one that was clicked

        buttons[i].setValue(0);
    }

    if (value)
        Label1.set("text", component.get("text") + " selected");
}

for (i = 0; i < NUM_BUTTONS; i++)
    buttons[i].setControlCallback(onButtonControl);
```

Key points:
- `buttons.length` adapts to however many buttons exist — no hard-coded count
- `continue` skips the triggering button
- `component.get("text")` reads the clicked button's label — no if/else branching
- `setControlCallback` is assigned in the same loop that populates the array

### indexOf

When you need to know *which* button was clicked (by position), use `indexOf`:

```javascript
inline function onButtonControl(component, value)
{
    local idx = buttons.indexOf(component);

    // Show the matching panel, hide all others
    for (i = 0; i < panels.length; i++)
        panels[i].set("visible", i == idx);
}
```

This is the standard pattern for multi-tab interfaces, mixer channels, articulation selectors — any set of parallel controls.

### The Result

The modular version is shorter, easier to read, and scales to any count without adding code. The pattern works with any component type: knobs, panels, sliders, combo boxes. Whenever you're writing the same code twice with only a name or index changing, it's time to refactor into array + loop + shared callback.


## What's Next

You now have the full language at your disposal. The next steps are applying it:

- **[Events & Messaging](/v2/guide/events-and-messaging)** — Broadcasters, reactive patterns, and scaling beyond simple callbacks
- **[Custom UI Design](/v2/guide/custom-ui)** — paint routines, Look and Feel, and the Graphics API
- **[Building a Sampler](/v2/guide/building-a-sampler)** — apply these patterns to a real instrument
- **Back to →** [Scripting Model](/v2/architecture/scripting-model) — the mental model this guide builds on