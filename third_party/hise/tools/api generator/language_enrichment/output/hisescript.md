---
title: HiseScript
description: "Variables, types, functions, and language features — everything you need to write HiseScript"

guidance:
  summary: >
    Complete guide to the HiseScript language. Part one covers the language
    itself: all five variable types (const var, var, reg, local, global) with
    realtime-safety implications, data types and type annotations, arrays and
    realtime-safe container alternatives (Buffer, MidiList, FixObjectArray,
    UnorderedStack), objects and the factory pattern, copy semantics and
    debugging tools (clone, trace, Script Watch Table), operators with
    short-circuit patterns, control flow, loops (for vs for...in performance),
    functions (inline vs regular with RT-safety reasoning, variable capturing),
    and namespaces and scope rules. Part two covers usage in HISE: callbacks
    and threading, script references to modules and components, and paint
    routines for custom graphics. Ends with a categorised
    differences-from-JavaScript reference. Assumes the reader has read the
    Scripting Model architecture page and understands the interface script
    pattern.
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
    - indexOf
    - setControlCallback
    - callbacks
    - script references
    - paint routines
    - Graphics object
    - Rectangle
    - mouse callbacks
  prerequisites:
    - scripting-model
    - module-tree
  complexity: intermediate
  architecture_counterpart: scripting-model
---

HiseScript is HISE's built-in scripting language. It uses JavaScript syntax but is its own language — designed for realtime audio safety. Several MIDI and timer callbacks run directly on the audio thread, where any operation that allocates memory, builds strings, or blocks will cause audible glitches. Many of the language features covered below — `reg`, `local`, `inline function` — exist specifically to give you tools that are safe to use in these contexts.

In practice, you will use HiseScript for three things:

- **Reacting to events** — MIDI input, UI interaction, timer ticks, and plugin initialisation all arrive through callbacks.
- **Controlling modules and components** — obtaining references to synths, modulators, effects, and UI controls, then reading and writing their parameters from script.
- **Drawing custom graphics** — paint routines on ScriptPanel components and Look and Feel functions for built-in components both use the same graphics object.

**See also:** [Usage in HISE](#usage-in-hise) -- callbacks, script references, and paint routines in practice


## The Language

### Variables

Variables store values in memory and give them a name you can reference throughout your script. HiseScript provides several variable types, each with different scope and performance characteristics.

Unlike standard JavaScript, HiseScript **requires** an explicit keyword (`var`, `const var`, `reg`, `local`, or `global`) when creating a variable. You cannot simply assign a value to an undeclared name — doing so produces a compile error. This prevents a notorious class of JavaScript bugs where a typo silently creates a new variable instead of updating the intended one:

```javascript
var myDatabaseEntry = "Hello";

// Much later in the code...
myDataBaseEntry = "Hello again"; // Compile error — capital B is a typo
```

In standard JavaScript this typo would silently create a second variable, leaving the original unchanged. In HiseScript the compiler catches it immediately. The only exception to this rule is the counter variable in a `for` loop, which can be used without a keyword for convenience (e.g., `for (i = 0; ...)`).

#### var

The most basic variable type. When declared in `onInit`, it is accessible from any callback in the script.

```javascript
var myVariable = 10;
myVariable = 20; // value can be changed
```

Variable names cannot start with a number and cannot use reserved keywords. In general, prefer the other variable types (`const var`, `reg`, `local`) when they fit your use case — `var` is the least efficient option, has scoping quirks inside namespaces (see [Namespaces](#namespaces)), and is **not realtime-safe** because it allocates memory at runtime. Never use `var` inside audio-thread callbacks (`onNoteOn`, `onNoteOff`, `onController`, `onTimer`).

#### const var

A constant variable — its value cannot be changed after declaration. By convention, constants holding simple values are written in `UPPER_CASE`; constants referencing UI components typically mirror the component's name.

```javascript
const var MY_CONSTANT = 10;
// MY_CONSTANT = 20;  // This would cause an error
```

> [!Tip:const var is your default variable type] When the script is compiled, HISE replaces every `const var` reference with its literal value — making it both safer and more efficient than other variable types. Use `const var` whenever a value does not need to change.

#### reg

Register variables are stored in a pre-allocated block of memory. Because the memory is allocated at compile time rather than at runtime, reading and writing a `reg` variable never triggers a memory allocation — making `reg` the correct choice for any mutable state used inside audio-thread callbacks.

```javascript
reg myReg = 10;
myReg = 20; // value can be changed, just like var
```

> [!Warning:Maximum 32 reg variables per scope] You can only declare 32 `reg` variables in the main script scope. Each namespace gets its own independent allocation of 32, so use namespaces to stay within the limit.

#### local

A local variable exists only within the scope (the curly braces) where it is declared. It cannot be accessed from other callbacks or functions.

```javascript
inline function processNote()
{
    local noteValue = Message.getNoteNumber();
    // noteValue only exists inside this function
}
```

Local variables cannot be declared in `onInit` (which has global scope by nature). They are used inside callbacks, inline functions, and code blocks where you need temporary storage without polluting the broader scope. Like `reg`, `local` variables inside an `inline function` are pre-allocated and realtime-safe.

#### global

Global variables enable communication between separate script processors in the same project. A variable declared with `global` is accessible from any script in the module tree.

```javascript
global myGlobal = 10;
```

This is equivalent to writing `Globals.myGlobal = 10`. It is good practice to always reference global variables using the `Globals.` prefix — this makes it immediately clear when reading the code that a variable is shared across scripts:

```javascript
Globals.currentArticulation = 2;
```

Any script can read or modify a global variable, so use them deliberately and keep track of which scripts depend on them.

#### Decision Framework

| Type | Lifetime | RT-Safe | Use for |
| --- | --- | --- | --- |
| `const var` | Script | Yes | Module refs, UI handles, lookup tables, anything immutable |
| `var` | Script | No | UI state (page index, mode flags) — scripting thread only |
| `reg` | Script | Yes | MIDI callback state (RR counters, last note, CC values) |
| `local` | Scope | Yes* | Temporaries, loop variables, function-internal state |
| `global` | Project | No | Cross-script communication |

*RT-safe inside `inline function` only.*


### Data Types

HiseScript supports five basic data types:

| Type | Description | Example |
| --- | --- | --- |
| **Number** | Integers and floating-point values (no distinction between them) | `58`, `3.14` |
| **String** | Text, enclosed in single or double quotes (not realtime-safe) | `"hello"`, `'world'` |
| **Boolean** | `true` (1) or `false` (0) | `true`, `false` |
| **undefined** | No value assigned | `var x;` (x is undefined) |
| **null** | Explicitly empty | `var x = null;` |

> [!Warning:Strings are not realtime-safe] String operations — concatenation, formatting, `Console.print()` — allocate memory and must never be used inside audio-thread callbacks in production. `Console.print()` is safe during development (it is stripped from exported plugins) but all other string operations must be avoided on the audio thread.

#### Checking if a Value is Defined

Use the built-in `isDefined()` function to check whether a variable has a value:

```javascript
if (isDefined(v))
    Console.print(v);

if (!isDefined(v))
    Console.print("v has no value");
```

This is more readable than `v != undefined` and is the idiomatic way to guard against undefined values in HiseScript.

#### Type Annotations

HiseScript is dynamically typed, but offers optional type annotations that constrain what type a variable may hold. When a type constraint is violated, HISE throws a compile error with zero runtime overhead in the exported plugin.

`reg` variables can be typed by appending a colon and the type identifier:

```javascript
reg:int noteNumber = 60;

noteNumber = 72;                   // OK
noteNumber = "not a number";       // Compile error
```

Common type identifiers: `int`, `double`, `number` (either), `string`, `Array`, `JSON`, `object`.

Type annotations also work on function parameters and return values (see [Functions](#functions)).


### Arrays

An array stores multiple values in a single variable, accessed by numeric index starting at zero. Array operations that change the array's size — `push()`, assigning beyond the current length — allocate memory and are **not realtime-safe**. If you need to work with collections on the audio thread, use pre-allocated arrays with a fixed size, or consider $API.Buffer$ for numeric data.

```javascript
var a = [1, 2, 3];           // declare an array
Console.print(a[0]);          // 1 — access by index (zero-based)
a[1] = 57;                    // replace the second element
a.push(99);                   // append to the end
Console.print(a.length);      // 4
```

Arrays can hold mixed types and can be nested:

```javascript
const var b = ["text", 3.14, [7, 8, 9]];
Console.print(b[2][0]); // 7
```

The full list of array methods is available in $API.Array$.

#### Realtime-Safe Containers

Standard arrays are flexible but not realtime-safe when resized. HISE provides several pre-allocated alternatives for audio-thread use:

| Type | Use case |
| --- | --- |
| **Buffer** | Fixed-size collection of floats — sample data, DSP, numeric processing |
| **MidiList** | 128-slot integer array — velocity maps, transposition tables, key switches |
| **FixObjectArray** | Fixed-size array of structured objects with a shared property layout |
| **FixObjectStack** | Same as above but with push/pop semantics |
| **UnorderedStack** | Pre-allocated stack of numbers — tracking active voices or note IDs |

Standard arrays are fine for initialisation and UI code. Consult the API Browser for details on each container type.

> [!Tip:Component arrays with loops] Arrays are the standard way to manage sets of similar UI components. Populate them with a loop in `onInit`:
> ```javascript
> const var NUM_BUTTONS = 3;
> const var buttons = [];
> for (i = 0; i < NUM_BUTTONS; i++)
>     buttons[i] = Content.getComponent("Button" + (i + 1));
> ```
> The parentheses around `(i + 1)` are essential — without them, `+` is interpreted as string concatenation (producing `"Button01"` instead of `"Button1"`).


### Objects

Objects store data as **key-value pairs**. Keys are strings, and values can be any type.

```javascript
const var o = { name: "Flute", velocity: 100, enabled: true };

Console.print(o.name);        // Flute — dot notation
Console.print(o["velocity"]); // 100   — bracket notation

o.velocity = 64;              // change a value
o.newKey = [1, 2, 3];         // add a new key
o.nested = { sub: 12 };       // add a nested object
Console.print(o.nested.sub);  // 12
```

Both dot notation and bracket notation work. Bracket notation is required when the key is stored in a variable.

#### Object Factories

HiseScript does not support JavaScript's `new` operator or prototype-based constructors. To create configured objects, use the **object factory** pattern:

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

In practice, the most common factory pattern wraps `Content.getComponent()`, configuring and returning an existing UI component:

```javascript
inline function createPanel(name, x, y)
{
    local p = Content.getComponent(name);
    p.set("x", x);
    p.set("y", y);
    return p;
}
```

This is idiomatic — UI elements must be created upfront during initialisation, so there is no need for dynamic object construction at runtime.


### Copying and Inspecting Data

> [!Warning:Assignment copies by reference] Assigning an array or object to another variable does not create a copy — both point to the same data. Modifying one modifies the other. Use `clone()` when you need an independent copy.

Arrays and objects are **passed by reference**:

```javascript
var a = ["dog", "cat", "fish"];
var b = a;

b[1] = "chicken";
Console.print(a[1]); // "chicken" — a was modified too!
```

#### clone()

When you need an independent copy, use `clone()`:

```javascript
var b = a.clone();
b[1] = "chicken";
Console.print(a[1]); // "cat" — a is unaffected
```

#### trace()

`trace()` converts an array or object into a readable string showing every element:

```javascript
const var o = { colors: ["red", "green", "blue"], gain: -6, enabled: true };
Console.print(trace(o));
```

> [!Tip:Built-in utility functions] `isDefined()`, `trace()`, and `clone()` are built into the language but do not appear in the API Browser or autocomplete — you need to know them by name. `trace()` is particularly useful for debugging LAF function parameters, API return values, and nested data structures.

#### Script Watch Table

The **Script Watch Table** (accessible from the toolbar) lets you inspect variable contents at a glance. Right-clicking an array or object expands it into a readable tree view. For variables declared inside functions (which do not appear in the watch table), use `trace()`.


### Operators

#### Arithmetic

| Operator | Operation | Example | Result |
| --- | --- | --- | --- |
| `+` | Addition | `10 + 5` | `15` |
| `-` | Subtraction | `10 - 5` | `5` |
| `*` | Multiplication | `10 * 5` | `50` |
| `/` | Division | `10 / 5` | `2` |
| `%` | Modulus (remainder) | `10 % 3` | `1` |
| `++` | Increment | `d++` | `d + 1` |
| `--` | Decrement | `d--` | `d - 1` |

Standard mathematical precedence applies. Use parentheses to override:

```javascript
var c = a * (b + 2); // addition happens first
```

#### String Concatenation

The `+` operator also joins strings, and converts numbers to strings when mixed:

```javascript
var s = "Value: " + 10;       // "Value: 10"
```

When HISE sees a string on either side of `+`, it treats the operation as concatenation rather than addition. This is important when constructing component names dynamically:

```javascript
// Wrong — concatenates "Button" + "0" + "1" = "Button01"
var name = "Button" + 0 + 1;

// Right — evaluates (0 + 1) = 1, then concatenates "Button" + "1" = "Button1"
var name = "Button" + (0 + 1);
```

#### Comparison

| Operator | Meaning |
| --- | --- |
| `==` | Equal to |
| `!=` | Not equal to |
| `>` | Greater than |
| `<` | Less than |
| `>=` | Greater than or equal |
| `<=` | Less than or equal |

Comparison operators return `1` (true) or `0` (false).

#### Logical

| Operator | Meaning |
| --- | --- |
| `&&` | AND — both sides must be true |
| `\|\|` | OR — at least one side must be true |
| `!` | NOT — inverts the result |

Any non-zero value is considered `true`. Only `0` is `false`.

Both `&&` and `||` use **short-circuit evaluation** — they stop as soon as the result is determined:

```javascript
var name = obj && obj.getName(); // null guard — only calls getName() if obj exists
var gain = userGain || -6;       // default — uses -6 if userGain is 0 or undefined
```


### Control Flow

#### if / else

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

#### Ternary Operator

A concise alternative for simple if/else logic:

```javascript
var colour = obj.value ? obj.bgColour : obj.itemColour1;
```

Widely used in LAF functions and anywhere a compact conditional assignment is needed.

#### switch

Cleaner than a chain of if/else when comparing a single value:

```javascript
switch (articulationIndex)
{
    case 0: sampler.loadSampleMap("Sustain"); break;
    case 1: sampler.loadSampleMap("Staccato"); break;
    case 2: sampler.loadSampleMap("Tremolo"); break;
    default: break;
}
```

> [!Warning:Every case needs break] Each `case` must end with `break;`. Without it, execution falls through into the next case silently.


### Loops

#### for

The standard counting loop. The counter does not need a keyword:

```javascript
for (i = 0; i < 10; i++)
    Console.print(i); // prints 0 through 9
```

Use index-based `for` loops when you need the index — for comparing positions, showing/hiding components by index, or writing values into an array at specific positions:

```javascript
for (i = 0; i < panels.length; i++)
    panels[i].set("visible", i == selectedIndex);
```

#### for...in

Iterates directly over the elements of a collection. On **arrays**, the loop variable receives each element's value; on **objects**, it receives each key:

```javascript
// Arrays — iterates values
const var names = ["Alice", "Bob", "Charlie"];
for (name in names)
    Console.print(name); // prints Alice, Bob, Charlie

// Objects — iterates keys
const var o = { a: 10, b: 20, c: 30 };
for (k in o)
    Console.print(o[k]); // prints 10, 20, 30
```

> [!Tip:Prefer for...in] `for...in` is significantly faster than an index-based `for` loop. Use it as your default whenever you do not need the index.

#### while

```javascript
var count = 0;
while (count < 20)
{
    Console.print(count);
    count++;
}
```

> [!Warning:Infinite loops freeze HISE] Ensure the condition will eventually become false — there is no watchdog to kill runaway loops. An infinite `while` loop locks the application and you lose unsaved work.

#### break and continue

`break` exits the loop entirely. `continue` skips to the next iteration:

```javascript
for (i = 0; i < values.length; i++)
{
    if (values[i] < 0)
        continue; // skip negative values

    if (values[i] == targetValue)
        break; // stop searching

    Console.print(values[i]);
}
```


### Functions

#### Inline Functions

Always use `inline function` for named, reusable functions — especially those called from audio-thread callbacks. Every time a regular `function` is called, it allocates a scope object on the heap. `inline function` avoids this entirely — its scope is pre-allocated at compile time, making it **realtime-safe**.

```javascript
inline function addition(a, b)
{
    local c = a + b;
    return c;
}

addition(50, 20); // 70
```

Inline function parameters and return values can be type-annotated:

```javascript
inline function setGain(value: number)
{
    // value is guaranteed to be a number
}

inline function: int getIndex()
{
    return 3;
}
```

#### Regular Functions

Use plain `function` when passing an anonymous callback that does not run on the audio thread — paint routines, dialog handlers, file operations:

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

**Rule:** named + reusable -> `inline function`. Anonymous + non-RT -> `function`. Note that `inline function` definitions cannot be nested inside other `inline function` definitions — define them at file or namespace scope.

#### Custom Callbacks

A custom callback is an inline function assigned to a specific UI control via `setControlCallback()`. Rather than routing all control interaction through `onControl`, each control gets its own dedicated handler:

```javascript
const var Knob1 = Content.getComponent("Knob1");

inline function onKnob1Control(component, value)
{
    Console.print(value);
}

Knob1.setControlCallback(onKnob1Control);
```

> [!Tip:Auto-generate callback boilerplate] Right-click a component in the Component List and select **Create custom callback definition** to generate the boilerplate automatically.

A key advantage of custom callbacks is that **multiple controls can share the same function**. The `component` parameter tells you which control was activated, allowing a single function to handle any number of controls — a pattern explored in [Writing Modular Code](#writing-modular-code).

#### Variable Capturing

> [!Warning:Closures require explicit capture] Inner functions cannot access outer function parameters automatically. You must list captured variables in square brackets or they will be `undefined` inside the callback.

When you define a callback function inside another function, the inner function cannot automatically access the outer function's parameters. HiseScript solves this with **explicit variable capturing**:

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

The `[presetName]` capture list makes `presetName` available inside the inner function. Capture multiple variables with commas: `function [a, b, c](ok)`. This pattern comes up frequently with dialog boxes, file operations, and any API call that takes a callback.


### Namespaces

Namespaces are HiseScript's primary organisational tool for structuring code. As a project grows beyond a handful of controls, keeping everything in a flat `onInit` callback becomes unwieldy. Namespaces let you divide your code into self-contained, labelled sections — each with its own scope.

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

This is the same dot-notation pattern used with HISE's built-in namespaces like `Console`, `Content`, and `Engine`. By convention, namespace names start with a capital letter.

A few rules:

- Namespaces **cannot be nested**
- Namespace names must not collide with HISE classes (`Engine`, `Content`, etc.)
- Each namespace gets its own allocation of **32 `reg` variables**, independent of the main scope

> [!Warning:var leaks out of namespaces] Variables declared with `var` inside a namespace leak into the global scope. Always use `reg`, `const var`, or `local` inside namespaces.

#### External Files

In practice, each namespace should live in its own `.js` file. Select the namespace block in the editor, right-click -> **Move selection to external file**, and give the file the same name as the namespace. HISE automatically adds an `include()` line to your `onInit` callback.

A well-organised project typically has an `onInit` that is nothing but a series of `include()` statements:

```javascript
include("App.js");
include("Paths.js");
include("LookAndFeel.js");
include("Header.js");
include("Settings.js");
```

Each file contains a single namespace.


## Usage in HISE

The previous section covered HiseScript as a language. This section covers how you use it inside HISE — responding to events, controlling the module tree and UI, and drawing custom graphics.

### Callbacks

HISE communicates with your script through six built-in callbacks, each tied to a specific event:

| Callback | Triggered by | Thread |
| --- | --- | --- |
| **onInit** | Plugin load or script compilation (`F5`) | Scripting |
| **onNoteOn** | Incoming MIDI note-on | Audio |
| **onNoteOff** | Incoming MIDI note-off | Audio |
| **onController** | Incoming MIDI CC message | Audio |
| **onTimer** | A configurable timer interval | Audio |
| **onControl** | User interaction with a UI control | Scripting |

The "Thread" column is critical for realtime safety: callbacks running on the audio thread must not allocate memory, build strings, or perform any operation that could block. Use `reg`, `local`, and `inline function` exclusively in these callbacks.

`onInit` is where you set up your script — declare variables, get references to modules and components, define functions, and configure callbacks. Everything in `onInit` runs once when the script compiles (or the plugin loads).

The MIDI callbacks (`onNoteOn`, `onNoteOff`, `onController`) are where you respond to incoming MIDI data. Use the `Message` namespace to read and modify the incoming event — for example, `Message.getNoteNumber()`, `Message.getVelocity()`, or `Message.setChannel()`.

#### onTimer

The `onTimer` callback runs on the audio thread and is primarily used for MIDI-synced timing tasks — arpeggiators, sequencers, and anything that needs to stay locked to the musical tempo. It is started with `Synth.startTimer(intervalInSeconds)` and stopped with `Synth.stopTimer()`. For UI-related timers (animations, periodic display updates), use `Engine.createTimerObject()` instead, which runs on the UI thread.

#### Custom Callbacks vs onControl

The `onControl` callback fires whenever any UI control changes value. In larger projects, routing all controls through a single function becomes unwieldy. Modern HISE projects typically assign individual **custom callbacks** to each control using `setControlCallback()`, as described in [Functions](#functions). This gives each control its own handler function, making code easier to organise and maintain.

> [!Tip:Shared callbacks with indexOf] When you have an array of similar controls (e.g. a row of buttons), assign the same callback to all of them and use `indexOf` to identify which one was triggered:
> ```javascript
> inline function onButtonControl(component, value)
> {
>     local idx = buttons.indexOf(component);
>     for (i = 0; i < panels.length; i++)
>         panels[i].set("visible", i == idx);
> }
> for (i = 0; i < buttons.length; i++)
>     buttons[i].setControlCallback(onButtonControl);
> ```
> This pattern scales to any number of controls — tab interfaces, mixer channels, articulation selectors.

**See also:** $MODULES.ScriptProcessor$ -- the MIDI script processor module, $MODULES.InterfaceScriptProcessor$ -- the interface script processor

### Script References

Script references let your script reach into HISE and control things. Before you can manipulate a module or a UI component from script, you need to obtain a **reference** to it.

All references should be declared as `const var` in `onInit`. HISE resolves them at compile time, which is both faster and safer than looking them up repeatedly at runtime.

#### Module References

HISE's module tree — synths, modulators, effects, MIDI processors — is controlled from script through references obtained with the `Synth` namespace:

```javascript
const var MySynth     = Synth.getChildSynth("Sine Wave Generator1");
const var MyModulator = Synth.getModulator("LFO Modulator1");
const var MyEffect    = Synth.getEffect("Delay1");
```

Each function takes the module's **Processor ID** (the name shown in its header bar) as a string argument. A quick way to get the correct reference code is to **right-click a module's header bar** in the main workspace and select **Create generic script reference**.

Once you have a reference, use `getAttribute()` and `setAttribute()` to read and write the module's parameters:

```javascript
Console.print(MySynth.getAttribute(MySynth.SaturationAmount));
MySynth.setAttribute(MySynth.SaturationAmount, 0.17);
```

The available parameter names for each module type are listed in the module reference and appear in the autocomplete popup when you type the reference name followed by a dot.

#### Component References

UI components — knobs, buttons, sliders, panels, labels — are referenced through `Content.getComponent()`:

```javascript
const var Knob1   = Content.getComponent("Knob1");
const var Button1 = Content.getComponent("Button1");
```

Right-click a component in the Component List or on the canvas (in edit mode) and select **Create script reference for selection**. If you select multiple components, HISE generates an array of references automatically.

Component **properties** (text, colour, position, visibility) are read and written with `.get()` and `.set()`:

```javascript
Knob1.set("text", "Saturation");
Console.print(Knob1.get("text")); // Saturation
```

The component's **value** (the thing that changes when a user turns a knob or clicks a button) is accessed with `.getValue()` and `.setValue()`:

```javascript
Console.print(Knob1.getValue()); // current knob position
Button1.setValue(1);              // turn the button on
```

#### Connecting Components to Modules

The most common scripting pattern in HISE is connecting a UI component to a module parameter:

```javascript
const var MySynth = Synth.getChildSynth("Sine Wave Generator1");
const var Knob1   = Content.getComponent("Knob1");

inline function onKnob1Control(component, value)
{
    MySynth.setAttribute(MySynth.SaturationAmount, value);
}

Knob1.setControlCallback(onKnob1Control);
```

Because this pattern is so common, HISE also offers a no-code shortcut: in the Property Editor, set a component's `processorId` and `parameterId` properties to link it directly to a module parameter without writing any callback code. The scripted approach gives you more flexibility (value scaling, conditional logic, updating multiple parameters), but the property shortcut is convenient for straightforward one-to-one connections.

**See also:** $API.Synth.getChildSynth$ -- obtain synth references, $API.Synth.getEffect$ -- obtain effect references, $API.Synth.getModulator$ -- obtain modulator references, $API.Content.getComponent$ -- obtain UI component references

### Paint Routines

Graphics code in HISE uses a **graphics object** (conventionally named `g`) to draw shapes, text, and colours onto a surface. You will encounter this object in two contexts: **ScriptPanel paint routines** (where you build custom controls from scratch) and **Look and Feel functions** (where you customise the appearance of built-in components). Both use the same drawing commands covered below.

#### Setting Up a Paint Routine

Call `setPaintRoutine()` on a panel reference and pass it a function with one parameter — the graphics object:

```javascript
const var Panel1 = Content.getComponent("Panel1");

Panel1.setPaintRoutine(function(g)
{
    g.fillAll(0xFF333333);
});
```

Once assigned, the paint routine controls the panel's appearance completely. If it draws nothing, the panel is invisible.

Colours are specified as hex integers in `0xAARRGGBB` format or using the `Colours` namespace. Most drawing commands take a `Rectangle` area parameter for positioning.

**See also:** $API.Colours$ -- colour constants and alpha helpers, $API.Rectangle$ -- area object with layout subdivision methods

#### The `this` Keyword

Inside a paint routine, `this` refers to the panel. This lets you write routines that adapt to the panel's size:

```javascript
Panel1.setPaintRoutine(function(g)
{
    var area = this.getLocalBounds(0);

    g.fillAll(0xFF333333);
    g.setColour(0xFFFFFFFF);
    g.drawRect(area, 2.0);
});
```

Because `this` resolves to whichever panel the routine is attached to, you can assign the same paint routine function to multiple panels and each will draw correctly at its own size.

#### Drawing Commands

The graphics object provides commands for shapes, text, and effects. Every `draw` command renders an outline; every `fill` command renders a solid shape. The active colour is set with `g.setColour()` and applies to all subsequent commands until changed.

| Command | Description |
| --- | --- |
| `fillAll(colour)` | Fills the entire surface |
| `setColour(colour)` | Sets the active colour |
| `fillRect(area)` | Fills a rectangle |
| `drawRect(area, borderSize)` | Draws a rectangle outline |
| `fillRoundedRectangle(area, cornerSize)` | Fills a rounded rectangle |
| `fillEllipse(area)` | Fills an ellipse (circle if width equals height) |
| `drawAlignedText(text, area, alignment)` | Draws text (`"left"`, `"centred"`, `"right"`) |
| `setFont(fontName, fontSize)` | Sets the font for subsequent text commands |

Commands are drawn in the order you write them — each new shape paints on top of everything before it. Structure your paint routine from **back to front**: background first, then borders, then foreground, then text.

#### Custom Fonts

Custom fonts are loaded once in `onInit` using `Engine.loadFontAs()`, then referenced by their ID:

```javascript
Engine.loadFontAs("{PROJECT_FOLDER}fonts/MyFont.ttf", "MyFont");

// Later, inside a paint routine:
g.setFont("MyFont", 24);
g.drawAlignedText("Hello", area, "centred");
```

#### Triggering a Repaint

A paint routine only runs when HISE knows the panel needs to be redrawn — typically at initialisation and when the panel's value changes. To trigger a repaint manually, call `Panel1.repaint()`, or `this.repaint()` from within a callback attached to the same panel.

#### Mouse Callbacks

Panels can respond to mouse input through `setMouseCallback()`, turning them into interactive controls. The callback receives an event object with properties like `e.clicked`, `e.mouseUp`, `e.hover`, `e.x`, and `e.y`. Before a panel responds to mouse events, set its **allowCallbacks** property in the Property Editor (`Clicks Only`, `Clicks & Hover`, or `All Callbacks`). By combining paint routines, mouse callbacks, and `this.data` for storing custom state, you can build any control imaginable.

**See also:** $API.ScriptPanel$ -- full ScriptPanel API, $API.ScriptLookAndFeel$ -- Look and Feel customisation API, $API.Graphics$ -- drawing commands reference, $API.Rectangle$ -- area object with layout subdivision

## Differences from JavaScript

HiseScript shares JavaScript's core syntax but diverges in specific areas. Most standard JS features work as expected: `===`/`!==`, `typeof`, array methods (`map`, `filter`, `find`, `forEach`, `every`, `some`, `concat`, `join`, `reverse`, `shift`, `pop`, `remove`, `contains`, and more), truthy/falsy rules, and string immutability are all identical. The differences below are grouped by rationale.

### Web/application concepts

These JavaScript features exist for building web applications with event loops, module systems, and class hierarchies. Audio scripting has different needs, so HiseScript omits them entirely.

| Feature | HiseScript equivalent |
| --- | --- |
| `async`/`await`, `Promise` | Callback-based API. There is no event loop. |
| `class Foo extends Bar`, `new`, prototype chain | Factory functions and namespaces. No prototype chain, no `new`, no inheritance. |
| `import`/`export` modules | `include("file.js")` — simple file inclusion, no module resolution |

### Deliberate design decisions

These differ from JavaScript intentionally. Each catches a class of bugs or improves reliability for audio code.

| Feature | JavaScript | HiseScript | Rationale |
| --- | --- | --- | --- |
| Variable declaration | `let`, `const`, `var` (implicit globals possible) | `const var`, `var`, `reg`, `local`, `global` (all explicit) | Each type has different allocation semantics for realtime safety |
| Scope | Block-scoped (`let`/`const`), function-scoped (`var`) | `var`/`reg` are script-scoped; `local` is block-scoped in functions | Explicit control over variable lifetime |
| Semicolons | Optional (ASI) | Required | No automatic semicolon insertion ambiguity |
| Closures | Functions capture surrounding scope implicitly | Explicit capture lists: `function [a, b](x)` | Deterministic memory, no hidden references |
| Arrow functions | `const f = (x) => x * 2` creates a closure | `x => x + 7` works but resolves as a normal function, not an `inline function` | Available syntactically, but not realtime-safe. Use `inline function` for audio callbacks. |
| `this` keyword | Dynamic binding, always available | Populated by HISE at specific call sites (paint routines, mouse callbacks, broadcaster listeners) | Not generally available — only meaningful where the host provides a context object |
| `for...in` | Iterates keys (including prototype chain) | Iterates values for arrays (like JS `for...of`), keys for objects. No prototype chain. | Simpler semantics, single construct for both use cases |
| Undefined arguments | Silently receives `undefined` | Throws a script error | Catches mismatched function calls that JS silently swallows |
| Regex | `/pattern/flags` literal syntax | Pass a string to $API.Engine.getRegexMatches$ | Avoids parser complexity. The string-based API is functionally equivalent. |
| `str.replace()` | Replaces first occurrence only | Replaces **all** occurrences | No need for `replaceAll()` or regex global flag |
| `arr.concat()` | Returns a new array | **Mutates** the original array in place | Avoids hidden allocation from copying |
| `typeof true` | Returns `"boolean"` | Returns `"number"` | No distinct boolean type at the language level |
| Division by zero | `1/0` returns `Infinity`, `0/0` returns `NaN` | Both return a non-finite value that propagates through arithmetic | Prevents `Infinity`/`NaN` from leaking into the audio signal path. Guard with `isFinite(x)`. |

### HISE-specific additions

Features HiseScript adds that do not exist in JavaScript.

| Feature | Description |
| --- | --- |
| `namespace` keyword | First-class code organisation. Primary structuring tool for larger scripts. |
| Type annotations | Optional compile-time type hints: `reg:int x = 5`. Zero runtime cost. |
| `inline function` | Realtime-safe function type that can be called from audio callbacks |
| `reg` and `local` keywords | Variable storage classes with specific allocation semantics for realtime use |


**See also:** $LANG.css$ -- CSS styling for UI components, $LANG.snex$ -- JIT-compiled DSP language for scriptnode, $LANG.regex$ -- regular expressions in HISE API calls
