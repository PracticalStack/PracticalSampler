# FixObjectFactory -- Class Analysis

## Brief
Factory defining typed memory layouts and creating fixed-object arrays and stacks.

## Purpose
FixObjectFactory defines a typed memory schema from a JSON prototype object and acts as a factory for creating memory-efficient containers (FixObjectArray, FixObjectStack) and individual objects (FixObject) that conform to that schema. Each property in the prototype determines a member's name, data type (int, float, bool), and default value. The factory also manages comparison functions used by its containers for sorting and lookup operations. All created containers share the factory's memory allocator and layout definition.

## Details

### Supported Data Types

The prototype JSON values determine the member data types:

| Prototype Value | Internal Type | C++ Storage | Size |
|----------------|---------------|-------------|------|
| Integer literal (e.g. `0`) | Integer | `int` | 4 bytes |
| Floating-point literal (e.g. `0.0`) | Float | `float` | 4 bytes |
| Boolean literal (`true`/`false`) | Boolean | `int` (read as bool) | 4 bytes |
| Array of numbers (e.g. `[0.0, 0.0, 0.0]`) | Array of inferred type | contiguous elements | 4 * N bytes |

Strings and objects are not supported as member types.

### Memory Layout

Properties are laid out in memory in the order they appear in the prototype object. Each member occupies a contiguous region at a computed byte offset. The layout is immutable after construction -- all objects and containers created from a factory share the same schema.

### Type Hash

A hash code computed from the concatenation of member IDs and type bytes (in layout order). Two factories with identical member names, types, and order produce the same hash. Available via `getTypeHash()`.

### Comparison Modes

See `setCompareFunction()` for the full comparator API, input modes, and performance characteristics. Summary:

| Mode | Input | Performance | Limit |
|------|-------|-------------|-------|
| Default | (none) | Byte-level equality, pointer ordering | -- |
| Single property | `"propertyName"` | Direct memory read at offset | 1 property |
| Multi-property | `"prop1,prop2"` | Sequential comparison in order | 2-4 properties |
| Custom function | `function(a, b)` | JS callback (synchronous) | No limit |

Setting a compare function propagates to all previously created arrays and stacks from this factory.

### Ownership Model

The factory retains references to all objects and containers it creates. The shared memory allocator (16-byte aligned) is reference-counted and lives as long as the factory or any of its children.

### Array-Typed Members

A prototype property whose value is a JSON array creates a fixed-size sub-array member. The array length is determined by the prototype array's length, and all elements must share a single numeric type (inferred from the first element). These members support bracket-indexed access on the resulting FixObject.

## obtainedVia
`Engine.createFixObjectFactory(layoutDescription)`

## minimalObjectToken
f

## Constants
| Name | Value | Type | Description | Group |
|------|-------|------|-------------|-------|
| prototype | (constructor arg) | JSON | The original layout description object passed to the factory constructor | Layout |

## Dynamic Constants
None.

## Common Mistakes

| Wrong | Right | Explanation |
|-------|-------|-------------|
| `Engine.createFixObjectFactory({ "name": "hello" })` | `Engine.createFixObjectFactory({ "id": 0, "value": 0.0 })` | String and object values are illegal member types. Only int, float, bool, and arrays of these are supported. |
| `f.setCompareFunction("a,b,c,d,e")` | `f.setCompareFunction(function(a, b) { ... })` | Multi-property string comparison is limited to 2-4 properties. Use a custom function for more. |

## codeExample
```javascript
// Create a factory with a typed layout
const var f = Engine.createFixObjectFactory({
    "id": 0,
    "velocity": 0.0,
    "active": false
});

// Create containers from the factory
const var list = f.createArray(128);
const var stack = f.createStack(16);

// Set an optimized comparator by property name
f.setCompareFunction("id");
```

## Alternatives
None. FixObjectFactory is the only mechanism for creating typed fixed-layout object containers.

## Related Preprocessors
None.

## Diagnostic Ideas
Reviewed: Yes
Count: 0
Rationale: All error conditions (invalid types, unknown properties, size limits) already produce immediate script errors via reportScriptError or Result::fail. No silent-failure preconditions exist.
