# An Introduction to HiseScript

HiseScript is HISE's built-in scripting language. It draws heavily from JavaScript in syntax and structure, but it is a language of its own — ES6 features are not supported, and there are important differences in variable types, scoping, and performance conventions. Developers with experience in JavaScript, Kontakt scripting, or general programming will find the transition relatively smooth.

Before diving into syntax, it helps to understand the three things you will use HiseScript for in practice.

**Reacting to events.** HISE communicates with your script through **callbacks** — functions that run automatically when something happens (MIDI input, UI interaction, timer ticks, plugin initialization). Your code goes inside these callbacks, and they are the starting point of all scripting in HISE. Callbacks are covered in the Practical Patterns chapter.

**Controlling modules and components.** HISE's module tree — synths, modulators, effects — and your UI components (knobs, buttons, panels) can all be controlled from script. You obtain a **reference** to a module or component, then use that reference to read and write its parameters or properties. This is how a knob on your interface ends up controlling a filter's cutoff frequency or a synth's gain. Script references are covered in the Practical Patterns chapter.

**Drawing custom graphics.** Graphics code in HISE appears in two equally important contexts. **Paint routines** on ScriptPanel components let you build fully custom controls from scratch — buttons, menus, sliders, visualizations. **Look and Feel (LAF) functions** let you customize the appearance of HISE's built-in components (knobs, buttons, combo boxes) without replacing them entirely. Both use the same graphics object and the same drawing commands, covered in the Practical Patterns chapter. The separate *LAF Introduction* guide covers the LAF-specific API in detail.

**Realtime safety.** Audio plugins must produce a continuous stream of samples without interruption. If code running on the audio thread takes too long — because it allocates memory, builds strings, or waits for a file — the result is audible glitches and dropouts. Standard JavaScript engines cannot guarantee this, which is why HISE uses its own scripting engine with language-level safeguards. The MIDI callbacks (`onNoteOn`, `onNoteOff`, `onController`) and `onTimer` all run on the audio thread. Many of the language features covered below — `reg`, `local`, `inline function` — exist specifically to give you tools that are safe to use in these contexts. Understanding this constraint is the single most important thing about writing HiseScript, and you will see realtime safety notes throughout this guide wherever a feature has audio-thread implications.

The following sections cover the language itself — starting with variables and building up to modular code patterns — before the Practical Patterns chapter shows how to apply them to real HISE work.

---

**Table of Contents**

**[The Language](#the-language)**

1. [Variables](#1-variables)
2. [Data Types](#2-data-types)
3. [Arrays](#3-arrays)
4. [Objects](#4-objects)
5. [Copying and Inspecting Data](#5-copying-and-inspecting-data)
6. [Operators](#6-operators)
7. [Control Flow](#7-control-flow)
8. [Loops](#8-loops)
9. [Functions](#9-functions)
10. [Namespaces](#10-namespaces)
11. [Writing Modular Code](#11-writing-modular-code)

**[Practical Patterns](#practical-patterns)**

1. [Callbacks](#1-callbacks)
2. [Script References](#2-script-references)
3. [Paint Routines](#3-paint-routines)

---

## The Language

### 1. Variables

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

Variable names cannot start with a number and cannot use reserved keywords. In general, prefer the other variable types (`const var`, `reg`, `local`) when they fit your use case — `var` is the least efficient option, has scoping quirks inside namespaces (see section 10), and is **not realtime-safe** because it allocates memory at runtime. Never use `var` inside audio-thread callbacks (`onNoteOn`, `onNoteOff`, `onController`, `onTimer`).

#### const var

A constant variable — its value cannot be changed after declaration. By convention, constants holding simple values are written in `UPPER_CASE`; constants referencing UI components typically mirror the component's name.

```javascript
const var MY_CONSTANT = 10;
// MY_CONSTANT = 20;  // This would cause an error
```

Constants are more than a safety measure. When the script is compiled, HISE replaces every reference to a `const var` with its literal value, making the script more efficient. Use `const var` whenever a value does not need to change.

#### reg

Register variables are stored in a pre-allocated block of memory that HISE can access without the overhead of a lookup table. Because the memory is allocated at compile time rather than at runtime, reading and writing a `reg` variable never triggers a memory allocation — making `reg` the correct choice for any mutable state used inside audio-thread callbacks.

```javascript
reg myReg = 10;
myReg = 20; // value can be changed, just like var
```

The limitation is that you can only declare 32 `reg` variables per scope.

#### local

A local variable exists only within the scope (the curly braces) where it is declared. It cannot be accessed from other callbacks or functions.

```javascript
function onNoteOn()
{
    local noteValue = Message.getNoteNumber();
    // noteValue only exists inside this function
}
```

Local variables cannot be declared in `onInit` (which has global scope by nature). They are used inside callbacks, inline functions (section 9), and code blocks where you need temporary storage without polluting the broader scope. Like `reg`, `local` variables inside an `inline function` are pre-allocated and realtime-safe.

#### global

Global variables enable communication between separate script processors in the same project. A variable declared with `global` is accessible from any script in the module tree.

```javascript
global myGlobal = 10;
```

Behind the scenes, this is equivalent to writing `Globals.myGlobal = 10`. It is good practice to always reference global variables using the `Globals.` prefix — this makes it immediately clear when reading the code that a variable is shared across scripts:

```javascript
Globals.currentArticulation = 2;
```

Any script can read or modify a global variable, so use them deliberately and keep track of which scripts depend on them.

---

### 2. Data Types

HiseScript supports five basic data types:

| Type | Description | Example |
|------|-------------|---------|
| **Number** | Integers and floating-point values (no distinction between them) | `58`, `3.14` |
| **String** | Text, enclosed in single or double quotes (not realtime-safe — see below) | `"hello"`, `'world'` |
| **Boolean** | `true` (1) or `false` (0) | `true`, `false` |
| **undefined** | No value assigned | `var x;` (x is undefined) |
| **null** | Explicitly empty | `var x = null;` |

In practice, `undefined` and `null` are rarely assigned intentionally. If you see an error about an "undefined parameter," it typically means a variable was used before being given a value.

String operations — concatenation, formatting, `Console.print()` — allocate memory and are **not realtime-safe**. Never use strings inside audio-thread callbacks (`onNoteOn`, `onNoteOff`, `onController`, `onTimer`). Using `Console.print()` in these callbacks during development is fine for debugging, but remove it before releasing your plugin.

#### Checking if a Value is Defined

When you are not certain whether a variable has been assigned, you can check with a comparison (`v != undefined`), but HiseScript provides a cleaner built-in alternative:

```javascript
if (isDefined(v))
    Console.print(v);
```

`isDefined()` returns `true` if the variable has a value and `false` if it is `undefined`. To check the inverse, prefix it with `!`:

```javascript
if (!isDefined(v))
    Console.print("v has no value");
```

This is more readable than `v != undefined` and is the idiomatic way to guard against undefined values in HiseScript.

#### Type Annotations

HiseScript is dynamically typed — a variable can hold any type and change type at any time. However, the language offers optional type annotations that let you constrain what type a variable may hold. When a type constraint is violated, HISE throws a compile error, catching bugs early without any runtime overhead in the exported plugin.

`reg` variables can be typed by appending a colon and the type identifier after the `reg` keyword:

```javascript
reg:int noteNumber = 60;

noteNumber = 72;                   // OK
noteNumber = "not a number";       // Compile error
```

The most commonly used type identifiers are:

| Identifier | Description |
|------------|-------------|
| `int` | Integer number |
| `double` | Floating-point number |
| `number` | Either `int` or `double` (preferred over the individual types in most cases) |
| `string` | A string value |
| `Array` | A JavaScript-style array |
| `JSON` | A JSON object |
| `object` | Either a JSON object or a HISE script object (e.g., a component reference) |

Type annotations are entirely optional and have no effect in the exported plugin — they exist purely as a development aid. The same annotation syntax can also be applied to function parameters and return values (see section 9).

The two compound data types — **arrays** and **objects** — are covered in their own sections below.

---

### 3. Arrays

An array stores multiple values in a single variable, accessed by numeric index starting at zero. Array operations that change the array's size — `push()`, assigning beyond the current length — allocate memory and are **not realtime-safe**. If you need to work with collections on the audio thread, use pre-allocated arrays with a fixed size, or consider `Buffer` for numeric data.

```javascript
var a = [1, 2, 3];           // declare an array
Console.print(a[0]);          // 1 — access by index (zero-based)
a[1] = 57;                    // replace the second element
a.push(99);                   // append to the end
Console.print(a.length);      // 4
Console.print(a);             // [1, 57, 3, 99]
```

Arrays can hold mixed types and can be nested:

```javascript
const var b = ["text", 3.14, [7, 8, 9]];
Console.print(b[2][0]); // 7
```

If you assign to an index beyond the current length, HISE fills any gap with `undefined`. The full list of array methods (`.indexOf()`, `.sort()`, `.reverse()`, `.remove()`, etc.) is available in the API Browser.

#### Other Container Types

Standard arrays are flexible but not realtime-safe when resized. HISE provides several pre-allocated alternatives for audio-thread use:

| Type | Use case |
|------|----------|
| **Buffer** | Fixed-size collection of floats — sample data, DSP, numeric processing |
| **MidiList** | 128-slot integer array — velocity maps, transposition tables, key switches |
| **FixObjectArray** | Fixed-size array of structured objects with a shared property layout |
| **FixObjectStack** | Same as above but with push/pop semantics |
| **UnorderedStack** | Pre-allocated stack of numbers — tracking active voices or note IDs |

Standard arrays are fine for initialization and UI code. Consult the API Browser for details on each container type.

---

### 4. Objects

Objects store data as **key-value pairs**. Keys are strings (though numeric keys must be quoted), and values can be any type.

```javascript
const var o = { name: "Flute", velocity: 100, enabled: true };

Console.print(o.name);        // Flute — dot notation
Console.print(o["velocity"]); // 100   — bracket notation

o.velocity = 64;              // change a value
o.newKey = [1, 2, 3];         // add a new key
o.nested = { sub: 12 };       // add a nested object
Console.print(o.nested.sub);  // 12
```

Both dot notation and bracket notation work. Bracket notation is required when the key is a number or stored in a variable. Values can be changed to a different type at any time — the type system is dynamic.

#### Object Factories (No `new` Operator)

HiseScript does not support JavaScript's `new` operator or prototype-based constructors. If you want a function that creates and returns a configured object, use the **object factory** pattern — create a plain object, set its properties, and return it:

```javascript
inline function createInstrument(name, velocity)
{
    var obj = {};
    obj.name = name;
    obj.velocity = velocity;
    obj.enabled = true;
    return obj;
}

const var flute = createInstrument("Flute", 100);
Console.print(flute.name); // Flute
```

In practice, the most common factory pattern in HISE involves grabbing a reference to an existing UI component with `Content.getComponent()`, configuring it, and returning it:

```javascript
inline function createPanel(name, x, y)
{
    var p = Content.getComponent(name);
    p.set("x", x);
    p.set("y", y);
    return p;
}

const var myPanel = createPanel("Panel1", 0, 0);
```

This is idiomatic HiseScript — UI elements and module references must be created upfront during initialization, so there is no need for dynamic object construction at runtime.

---

### 5. Copying and Inspecting Data

Arrays and objects in HiseScript are **passed by reference**. Assigning one to another does not create a copy — both variables point to the same underlying data:

```javascript
var a = ["dog", "cat", "fish"];
var b = a;

b[1] = "chicken";
Console.print(a[1]); // "chicken" — a was modified too!
```

This applies equally to objects. It is a common source of subtle bugs whenever you intend to work with a separate copy.

#### clone()

When you need an independent copy, use `clone()`:

```javascript
var b = a.clone();

b[1] = "chicken";
Console.print(a[1]); // "cat" — a is unaffected
```

`clone()` creates a deep copy and works on both arrays and objects. Use it any time you need to duplicate data without linking the original and the copy.

#### trace()

`trace()` converts an array or object into a readable string representation, showing every element and its index or key:

```javascript
const var o = { colors: ["red", "green", "blue"], gain: -6, enabled: true };
Console.print(trace(o));
```

This is indispensable when debugging complex data structures — LAF function parameters, API return values, or any nested object whose contents you need to verify.

Note that `isDefined()`, `trace()`, and `clone()` are not listed in the API Browser or autocomplete. They are built into the language but must be known by name.

#### Script Watch Table

The **Script Watch Table** (accessible from the toolbar) lets you inspect variable contents at a glance. Right-clicking an array or object variable in the watch table expands it into a readable tree view. For variables declared inside functions (which do not appear in the watch table), use `trace()` as described above.

---

### 6. Operators

#### Arithmetic

| Operator | Operation | Example | Result |
|----------|-----------|---------|--------|
| `+` | Addition | `10 + 5` | `15` |
| `-` | Subtraction | `10 - 5` | `5` |
| `*` | Multiplication | `10 * 5` | `50` |
| `/` | Division | `10 / 5` | `2` |
| `%` | Modulus (remainder) | `10 % 3` | `1` |
| `++` | Increment | `d++` | `d + 1` |
| `--` | Decrement | `d--` | `d - 1` |

Standard mathematical precedence applies (multiplication before addition). Use parentheses to override:

```javascript
var c = a * (b + 2); // addition happens first
```

#### String Concatenation

The `+` operator also joins strings, and will convert numbers to strings when mixed:

```javascript
var s = "Hello " + "World";   // "Hello World"
var s = "Value: " + 10;       // "Value: 10"
```

When HISE sees a string on either side of `+`, it treats the operation as concatenation rather than addition. This is important to understand when constructing component names dynamically (covered in section 11).

#### Comparison

| Operator | Meaning | Example |
|----------|---------|---------|
| `==` | Equal to | `a == b` |
| `!=` | Not equal to | `a != b` |
| `>` | Greater than | `a > b` |
| `<` | Less than | `a < b` |
| `>=` | Greater than or equal | `a >= b` |
| `<=` | Less than or equal | `a <= b` |

Comparison operators return `1` (true) or `0` (false).

#### Logical

| Operator | Meaning | Description |
|----------|---------|-------------|
| `&&` | AND | Both sides must be true |
| `\|\|` | OR | At least one side must be true |
| `!` | NOT | Inverts the result |

```javascript
if (a == 10 && b == 5)  // true only if both conditions are met
if (a == 10 || b == 5)  // true if either condition is met
if (!(a == 10))         // true if a is NOT 10
```

Any non-zero value is considered `true` in a logical context. Only `0` is `false`.

Both `&&` and `||` use **short-circuit evaluation** — they stop as soon as the result is determined. This makes them useful beyond `if` statements. `&&` can serve as a null guard, only evaluating the right side when the left side is truthy:

```javascript
var name = obj && obj.getName(); // only calls getName() if obj exists
```

`||` can provide default values, returning the right side when the left side is falsy:

```javascript
var gain = userGain || -6; // use -6 if userGain is 0 or undefined
```

---

### 7. Control Flow

#### if / else if / else

The basic decision-making structure:

```javascript
if (a == 10)
    Console.print("ten");
else if (a == 5)
    Console.print("five");
else
    Console.print("something else");
```

When an `if` or `else` block contains only a single line, the curly braces are optional. For multi-line blocks, braces are required:

```javascript
if (a == 10)
{
    Console.print("ten");
    b = 20;
}
```

#### Ternary Operator

A concise alternative for simple if/else logic. The syntax is `condition ? valueIfTrue : valueIfFalse`:

```javascript
var colour = obj.value ? obj.bgColour : obj.itemColour1;
```

This is equivalent to:

```javascript
var colour;
if (obj.value)
    colour = obj.bgColour;
else
    colour = obj.itemColour1;
```

The ternary operator is widely used in LAF functions and anywhere a compact conditional assignment is needed.

#### switch

When comparing a single value against many possible cases, a `switch` statement is cleaner than a chain of `if/else if`:

```javascript
switch (a)
{
    case 10: Console.print("Bob"); break;
    case 8:  Console.print("Sarah"); break;
    case 6:  Console.print("John"); break;
    default: Console.print("Unknown"); break;
}
```

Each `case` must end with `break;` to prevent fall-through. The `default` case handles any value not matched above. You can nest `if` statements inside a `case` if needed.

---

### 8. Loops

#### for

The standard counting loop. It declares a counter variable, specifies a condition, and increments each iteration:

```javascript
for (i = 0; i < 10; i++)
{
    Console.print(i); // prints 0 through 9
}
```

The counter does not need to be declared with `var` — HiseScript handles it automatically within the loop scope. Loops can count in any direction and with any step size:

```javascript
for (i = 10; i >= 1; i--)
    Console.print(i); // prints 10 down to 1
```

Use index-based `for` loops when you need the index of each element — for example, when comparing positions, showing/hiding components by index, or writing values back into an array at specific positions:

```javascript
for (i = 0; i < panels.length; i++)
    panels[i].set("visible", i == selectedIndex);
```

#### for...in

The `for...in` loop iterates directly over the elements of a collection. On **arrays** and **Buffers**, the loop variable receives each element's value; on **objects**, it receives each key:

```javascript
// Arrays — iterates elements directly
const var names = ["Alice", "Bob", "Charlie"];

for (name in names)
    Console.print(name); // prints Alice, Bob, Charlie

// Objects — iterates keys
const var o = { a: 10, b: 20, c: 30 };

for (k in o)
    Console.print(o[k]); // prints 10, 20, 30
```

`for...in` is **significantly faster** than an index-based `for` loop and should be your default choice whenever you do not need the index. Use the index-based `for` loop only when the position of each element matters (e.g., comparing `i` against another value, or writing back to a specific index).

#### while

Repeats a block as long as a condition is true:

```javascript
var count = 0;
while (count < 20)
{
    Console.print(count);
    count++;
}
```

Be careful to ensure the condition will eventually become false — otherwise the loop runs forever and locks the application. In practice, `for` and `for...in` loops are more commonly used in HiseScript.

#### break and continue

Two keywords let you alter the flow inside a loop. `break` exits the loop entirely, stopping all further iterations. `continue` skips the remainder of the current iteration and jumps to the next one:

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

---

### 9. Functions

A function is a named, reusable block of code. It can accept input (parameters), perform a task, and optionally return a result.

#### Built-in Functions

HISE provides a large library of built-in functions organized by namespace. The **API Browser** (accessible from the left panel) lists all available functions, grouped and searchable. Right-clicking a function entry shows its documentation; double-clicking inserts it into the editor.

Autocomplete is also available: type a namespace followed by a dot (e.g., `Engine.`) and press Escape to browse available functions.

#### Inline Functions

In HiseScript, you should use `inline function` rather than plain `function` for any named, reusable function — especially those called from audio-thread callbacks like `onNoteOn` and `onController`. Every time a regular `function` is called, it allocates a scope object on the heap. On the audio thread, this allocation can cause dropouts. `inline function` avoids this entirely — its scope is pre-allocated at compile time, making it realtime-safe. Inline functions also support `local` variable declarations.

```javascript
inline function addition(a, b)
{
    local c = a + b;
    Console.print(c);
}

addition(50, 20); // prints 70
addition(10, 5);  // prints 15
```

Inline function parameters and return values can be type-annotated using the same syntax described in section 2 (Data Types). Append a colon and a type identifier to a parameter name to constrain its type, or place the type between the colon and the function name for the return value:

```javascript
inline function setGain(value: number)
{
    // value is guaranteed to be a number (int or double)
}

inline function: int getIndex()
{
    return 3;
}
```

You can annotate individual parameters selectively — any parameter without a type annotation remains dynamically typed. As with `reg` type annotations, the checks only run inside HISE and have zero overhead in the exported plugin.

#### Regular Functions

Plain `function` declarations (without `inline`) are commonly used when passing an anonymous function as an argument in a non-realtime context. Paint routines, mouse callbacks, dialog handlers, and similar API calls all expect a function parameter — and since these do not run on the audio thread, the overhead of a regular function is irrelevant:

```javascript
Panel1.setPaintRoutine(function(g)
{
    g.fillAll(Colours.red);
});

Engine.showYesNoWindow("Confirm", "Are you sure?", function(ok)
{
    if (ok)
        Console.print("Confirmed");
});
```

When you need a quick, throwaway function that is only used in one place, plain `function` is more concise than `inline function`. For named, reusable functions — and anything called from the audio thread — always use `inline function`. Never use a plain `function` declaration for code that runs in `onNoteOn`, `onNoteOff`, `onController`, or `onTimer`.

#### Best Practices

- **One job per function** — a function should do a single, well-defined task
- **Use `local` variables inside functions** — keep function-internal state out of the global scope

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

You can right-click a component in the component list and select **Create custom callback definition** to generate the boilerplate automatically.

A key advantage of custom callbacks is that **multiple controls can share the same function**. The `component` parameter tells you which control was activated, allowing a single function to handle any number of controls — a pattern explored in detail in section 11.

#### Variable Capturing

When you define a callback function inside another function, the inner function cannot automatically access the outer function's parameters or local variables. This is a common scenario — for example, when showing a confirmation dialog from within a function:

```javascript
inline function deletePreset(presetName)
{
    Engine.showYesNoWindow("Confirm", "Delete preset?", function(ok)
    {
        Console.print(presetName); // ERROR — presetName is not accessible here
    });
}
```

The inner callback does not know about `presetName` because it runs in a separate scope. HiseScript solves this with **explicit variable capturing**, borrowed from C++ lambda syntax. List the variables you need inside square brackets between the `function` keyword and the parameter list:

```javascript
inline function deletePreset(presetName)
{
    Engine.showYesNoWindow("Confirm", "Delete preset?", function [presetName](ok)
    {
        if (ok)
            Console.print("Deleting: " + presetName); // Works!
    });
}
```

The `[presetName]` capture list tells HiseScript to make `presetName` available inside the inner function. You can capture multiple variables by separating them with commas: `function [a, b, c](ok)`.

This pattern comes up frequently with asynchronous operations like dialog boxes, file operations, and any API call that takes a callback function as an argument.

---

### 10. Namespaces

Namespaces are HiseScript's primary organizational tool for structuring code. As a project grows beyond a handful of controls, keeping everything in a flat `onInit` callback becomes unwieldy. Namespaces let you divide your code into self-contained, labeled sections — each with its own scope.

```javascript
namespace App
{
    reg myValue = 10;

    inline function myFunction()
    {
        Console.print("Hello World");
    }
}

// Access from outside using the namespace name
Console.print(App.myValue);
App.myFunction();
```

This should feel familiar — it is the same dot-notation pattern used with HISE's built-in namespaces like `Console`, `Content`, and `Engine`. By convention, namespace names start with a capital letter. Treat the inside of a namespace like `onInit` — declare variables, get component references, define functions, and set up callbacks.

A few rules: namespaces cannot be nested; namespace names must not collide with HISE classes (`Engine`, `Content`, etc.); and variables declared with `var` inside a namespace will leak into the global scope — use `reg`, `const var`, or `local` instead. Each namespace also gets its own allocation of 32 `reg` variables, independent of the main scope and every other namespace.

#### External Files

In practice, each namespace should live in its own `.js` file. Select the namespace block in the editor, right-click > **Move selection to external file**, and give the file the same name as the namespace. HISE automatically adds an `include()` line to your `onInit` callback.

A well-organized project typically has an `onInit` that is nothing but a series of `include()` statements:

```javascript
include("App.js");
include("Paths.js");
include("LookAndFeel.js");
include("Header.js");
include("Settings.js");
include("MainPanel.js");
```

Each file contains a single namespace. This makes code easy to locate, reusable across projects, and independently maintainable.

---

### 11. Writing Modular Code

The concepts from the previous sections — arrays, loops, functions, and namespaces — come together to form a pattern that is central to practical HiseScript development. Rather than writing repetitive code for each UI control, you store references in arrays and operate on them with loops.

#### The Problem

Consider an interface with three buttons and a label. When a button is clicked, the label should display which button was clicked, and the other buttons should turn off (radio-group behavior, but with the ability to deselect all).

A naive approach creates a separate callback for each button with nearly identical code. This works, but each additional button requires duplicating the callback and manually adding lines to turn off every other button. At three buttons, it is manageable. At ten, it becomes tedious and error-prone.

#### Arrays of Components

The first step toward modularity is storing component references in an array, populated by a loop:

```javascript
const var NUM_BUTTONS = 3;
var buttons = [];

for (i = 0; i < NUM_BUTTONS; i++)
    buttons[i] = Content.getComponent("Button" + (i + 1));
```

The expression `"Button" + (i + 1)` constructs the component name dynamically. The parentheses around `(i + 1)` are essential — without them, `+` would be interpreted as string concatenation (producing `"Button01"` instead of `"Button1"`), because a string appears on the left side.

If you later add more buttons to the interface, the only change required is updating `NUM_BUTTONS`.

#### Shared Callback Function

Instead of writing one callback per button, write a single function and assign it to all buttons in the same loop:

```javascript
inline function onButtonControl(component, value)
{
    for (i = 0; i < buttons.length; i++)
    {
        if (buttons[i] == component)
            continue;

        buttons[i].setValue(0);
    }

    if (value)
        Content.getComponent("Label1").set("text", component.get("text") + " was clicked");
}

for (i = 0; i < NUM_BUTTONS; i++)
    buttons[i].setControlCallback(onButtonControl);
```

Several things to note:

- **`buttons.length`** adapts the loop to however many buttons are in the array, so you never hard-code the count inside the function.
- **`continue`** skips the button that triggered the callback, avoiding an unnecessary off/on toggle.
- **`component.get("text")`** retrieves the clicked button's display text, so the label message is always correct without any if/else branching on specific button names.
- **`setControlCallback`** is called inside the same loop that populates the array, keeping everything in one place.

#### indexOf

When you need to know *which* button in the array triggered the callback (not just that it was one of them), use `indexOf`:

```javascript
local idx = buttons.indexOf(component);
```

This returns the array index of the matching component, or `-1` if it is not found. This is particularly useful for **multi-tab interfaces**, where each button corresponds to a panel — knowing the index lets you show the correct panel and hide the others in a single loop:

```javascript
for (i = 0; i < panels.length; i++)
    panels[i].set("visible", i == idx);
```

#### The Result

The modular version of the script is shorter, easier to read, and scales to any number of buttons without adding lines of code. The pattern generalizes beyond buttons — it works with any set of similar controls: knobs, panels, sliders, combo boxes. Whenever you find yourself writing the same code two or three times with only a name or index changing, it is a strong signal to refactor into an array-and-loop structure.

#### Code Style Notes

- **Single-line if statements** can omit curly braces: `if (value) doSomething();` — but only when there is exactly one statement.
- **Reducing line count is not a goal in itself.** The purpose is clarity and maintainability. Do not compress code at the expense of readability.

---

## Practical Patterns

### 1. Callbacks

HISE communicates with your script through six built-in callbacks, each tied to a specific event:

| Callback | Triggered by | Thread |
|----------|-------------|--------|
| **onInit** | Plugin load or script compilation (`F5`) | Scripting |
| **onNoteOn** | Incoming MIDI note-on | Audio |
| **onNoteOff** | Incoming MIDI note-off | Audio |
| **onController** | Incoming MIDI CC message | Audio |
| **onTimer** | A configurable timer interval | Audio |
| **onControl** | User interaction with a UI control | Scripting |

The "Thread" column is critical for realtime safety: callbacks running on the audio thread must not allocate memory, build strings, or perform any operation that could block. Use `reg`, `local`, and `inline function` exclusively in these callbacks (see the realtime safety notes throughout The Language chapter).

`onInit` is where you set up your script — declare variables, get references to modules and components, define functions, and configure callbacks. Everything in `onInit` runs once when the script compiles (or the plugin loads).

The MIDI callbacks (`onNoteOn`, `onNoteOff`, `onController`) are where you respond to incoming MIDI data. Use the `Message` namespace to read and modify the incoming event — for example, `Message.getNoteNumber()`, `Message.getVelocity()`, or `Message.setChannel()`.

#### onTimer

The `onTimer` callback runs on the audio thread and is primarily used for MIDI-synced timing tasks — arpeggiators, sequencers, and anything that needs to stay locked to the musical tempo. It is started with `Synth.startTimer(intervalInSeconds)` and stopped with `Synth.stopTimer()`. For UI-related timers (animations, periodic display updates), use `Engine.createTimerObject()` instead, which runs on the UI thread and avoids blocking audio processing.

#### Custom Callbacks vs onControl

The `onControl` callback fires whenever any UI control changes value. In larger projects, routing all controls through a single function becomes unwieldy. Modern HISE projects typically assign individual **custom callbacks** to each control using `setControlCallback()`, as described in section 9 of The Language. This gives each control its own handler function, making code easier to organize and maintain.

---

### 2. Script References

Script references let your script reach into HISE and control things. Before you can manipulate a module or a UI component from script, you need to obtain a **reference** to it.

All references should be declared as `const var` in `onInit`. HISE resolves them at compile time, which is both faster and safer than looking them up repeatedly at runtime.

#### Module References

HISE's module tree — synths, modulators, effects, MIDI processors — is controlled from script through references obtained with the `Synth` namespace:

```javascript
const var MySynth     = Synth.getChildSynth("Sine Wave Generator1");
const var MyModulator = Synth.getModulator("LFO Modulator1");
const var MyEffect    = Synth.getEffect("Delay1");
```

Each function takes the module's **Processor ID** (the name shown in its header bar) as a string argument. A quick way to get the correct reference code is to **right-click a module's header bar** in the main workspace and select **Create generic script reference** — this copies a ready-to-paste `const var` declaration to your clipboard.

Once you have a reference, use `getAttribute()` and `setAttribute()` to read and write the module's parameters. The parameter is identified using the reference variable followed by a dot and the parameter name:

```javascript
// Read the current value
Console.print(MySynth.getAttribute(MySynth.SaturationAmount));

// Set it to 17%
MySynth.setAttribute(MySynth.SaturationAmount, 0.17);
```

The available parameter names for each module type are listed in the HISE Modules documentation and appear in the autocomplete popup when you type the reference name followed by a dot.

#### Component References

UI components — knobs, buttons, sliders, panels, labels — are referenced through `Content.getComponent()`:

```javascript
const var Knob1   = Content.getComponent("Knob1");
const var Button1 = Content.getComponent("Button1");
```

As with modules, the quickest way to generate this code is to **right-click a component** in the Component List or on the canvas (in edit mode) and select **Create script reference for selection**. If you select multiple components, HISE generates an array of references automatically.

Component **properties** (text, color, position, visibility, etc.) are read and written with `.get()` and `.set()`:

```javascript
Knob1.set("text", "Saturation");
Console.print(Knob1.get("text")); // Saturation
```

The component's **value** (the thing that changes when a user turns a knob or clicks a button) is accessed with `.getValue()` and `.setValue()`:

```javascript
Console.print(Knob1.getValue()); // current knob position
Button1.setValue(1);              // turn the button on
```

What `.getValue()` returns depends on the component type: a 0–1 range for sliders, 0 or 1 for buttons, an array index for combo boxes, and so on.

#### Connecting Components to Modules

The most common scripting pattern in HISE is connecting a UI component to a module parameter — turning a knob should change a synth's setting. This brings together callbacks, functions, and script references:

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

---

### 3. Paint Routines

Graphics code in HISE uses a **graphics object** (conventionally named `g`) to draw shapes, text, and colors onto a surface. You will encounter this object in two contexts: **ScriptPanel paint routines** (where you build custom controls from scratch) and **Look and Feel functions** (where you customize the appearance of built-in components). Both use the same drawing commands covered below.

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

#### Colors

Colors can be specified as hex integers in `0xAARRGGBB` format (`AA` = alpha, `RR` = red, `GG` = green, `BB` = blue), or using the `Colours` namespace:

```javascript
g.setColour(0xFFFF0000);       // opaque red
g.setColour(Colours.white);    // named constant
g.setColour(Colours.withAlpha(Colours.dodgerblue, 0.5)); // 50% transparent
```

#### The Rectangle Object

Most drawing commands take an area parameter that defines where to draw. While you can pass a plain `[x, y, width, height]` array, the `Rectangle` object is the preferred approach — it wraps JUCE's `Rectangle<float>` class and provides methods for subdivision and layout:

```javascript
var area = Rectangle(10, 10, 200, 100);

g.fillRect(area);                        // fill the rectangle
g.fillRect(area.reduced(5));             // fill with 5px inset on all sides
g.fillRect(area.translated(50, 0));      // shifted 50px to the right
```

The key layout methods are:

| Method | Description |
|--------|-------------|
| `reduced(amount)` | Returns a smaller rectangle, inset by the given amount on all sides |
| `removeFromTop(amount)` | Removes and returns a strip from the top, shrinking the original |
| `removeFromLeft(amount)` | Removes and returns a strip from the left, shrinking the original |
| `removeFromBottom(amount)` | Removes and returns a strip from the bottom |
| `removeFromRight(amount)` | Removes and returns a strip from the right |
| `translated(dx, dy)` | Returns a rectangle shifted by the given offset |
| `withHeight(h)` | Returns a rectangle with the same position but a different height |
| `withSizeKeepingCentre(w, h)` | Returns a rectangle resized around its centre point |

The `removeFrom...` methods are particularly powerful for layout — each call slices off a portion and modifies the original rectangle in place, so you can progressively divide an area into rows or columns.

#### The `this` Keyword

Inside a paint routine, `this` refers to the panel. This lets you write routines that adapt to the panel's size:

```javascript
Panel1.setPaintRoutine(function(g)
{
    var area = this.getLocalBounds(0); // [0, 0, width, height] as an array

    g.fillAll(0xFF333333);
    g.setColour(0xFFFFFFFF);
    g.drawRect(area, 2.0);
});
```

Because `this` resolves to whichever panel the routine is attached to, you can assign the same paint routine function to multiple panels and each will draw correctly at its own size.

#### Drawing Commands

The graphics object provides commands for shapes, text, and effects. Every `draw` command renders an outline; every `fill` command renders a solid shape. The active color is set with `g.setColour()` and applies to all subsequent commands until changed.

| Command | Description |
|---------|-------------|
| `fillAll(colour)` | Fills the entire surface |
| `setColour(colour)` | Sets the active color |
| `fillRect(area)` | Fills a rectangle |
| `drawRect(area, borderSize)` | Draws a rectangle outline |
| `fillRoundedRectangle(area, cornerSize)` | Fills a rounded rectangle |
| `fillEllipse(area)` | Fills an ellipse (circle if width equals height) |
| `drawAlignedText(text, area, alignment)` | Draws text (`"left"`, `"centred"`, `"right"`) |
| `setFont(fontName, fontSize)` | Sets the font for subsequent text commands |

Commands are drawn in the order you write them — each new shape paints on top of everything before it. Structure your paint routine from **back to front**: background first, then borders, then foreground, then text. For the full list of drawing commands, type `Graphics` in the API Browser.

#### Custom Fonts

Custom fonts are loaded once in `onInit` using `Engine.loadFontAs()`, then referenced by their ID:

```javascript
Engine.loadFontAs("{PROJECT_FOLDER}fonts/MyFont.ttf", "MyFont");

// Later, inside a paint routine:
g.setFont("MyFont", 24);
g.drawAlignedText("Hello", area, "centred");
```

#### Triggering a Repaint

A paint routine only runs when HISE knows the panel needs to be redrawn — typically at initialization and when the panel's value changes. To trigger a repaint manually, call `Panel1.repaint()`, or `this.repaint()` from within a callback attached to the same panel.

#### Mouse Callbacks

Panels can respond to mouse input through `setMouseCallback()`, turning them into interactive controls. The callback receives an event object with properties like `e.clicked`, `e.mouseUp`, `e.hover`, `e.x`, and `e.y`. Before a panel responds to mouse events, set its **allowCallbacks** property in the Property Editor (`Clicks Only`, `Clicks & Hover`, or `All Callbacks`). By combining paint routines, mouse callbacks, and `this.data` for storing custom state, you can build any control imaginable.

---

*This guide covers the core of HiseScript. For a complete API reference, use the built-in API Browser in HISE, or consult the [official documentation](https://docs.hise.audio). The [HISE Forum](https://forum.hise.audio) is the best place to ask questions and see real-world examples.*