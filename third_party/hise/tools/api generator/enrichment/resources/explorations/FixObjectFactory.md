# FixObjectFactory -- C++ Source Exploration

## Resources Consulted
- `enrichment/resources/survey/class_survey_data.json` (FixObjectFactory entry)
- `enrichment/resources/survey/class_survey.md` (creation chain)
- No prerequisite classes (FixObjectFactory is a root factory)
- No existing base class explorations apply

## Source Files
- **Header:** `HISE/hi_scripting/scripting/api/FixLayoutObjects.h` (lines 470-513)
- **Implementation:** `HISE/hi_scripting/scripting/api/FixLayoutObjects.cpp` (lines 304-507)
- **Factory call site:** `HISE/hi_scripting/scripting/api/ScriptingApi.cpp` (line 2169-2171)

## Namespace Structure

Everything lives in `hise::fixobj`. The namespace contains:

| Class | Role | Script Name |
|-------|------|-------------|
| `Allocator` | Internal memory allocator with 16-byte aligned blocks | (not exposed) |
| `LayoutBase` | Base class providing memory layout definition | (not exposed) |
| `ObjectReference` | Individual typed object with named member access | `FixObject` |
| `Array` | Fixed-size array of ObjectReference items | `FixObjectArray` |
| `Stack` | Variable-occupancy stack (extends Array) | `FixObjectStack` |
| `Factory` | Schema definer + container factory | `FixObjectFactory` |

## Class Declaration: fixobj::Factory

```cpp
struct Factory : public LayoutBase,
                 public ConstScriptingObject
{
    Factory(ProcessorWithScriptingContent* s, const var& d);
    Identifier getObjectName() const override { RETURN_STATIC_IDENTIFIER("FixObjectFactory"); };

    // API methods
    var create();
    var createArray(int numElements);
    var createStack(int numElements);
    void setCompareFunction(var newCompareFunction);
    int getTypeHash() const { return typeHash; }

    int compare(ObjectReference::Ptr v1, ObjectReference::Ptr v2);

private:
    struct Wrapper;
    WeakCallbackHolder customCompareFunction;
    ObjectReference::CompareFunction compareFunction;
    ReferenceCountedArray<ObjectReference> singleObjects;
    ReferenceCountedArray<Array> arrays;
};
```

### Inheritance Chain
- `LayoutBase` -- provides `allocator`, `layout` (MemoryLayoutItem::List), `typeHash`, `getElementSizeInBytes()`, `createLayout()`
- `ConstScriptingObject` -- provides scripting API registration (`ADD_API_METHOD_N`, `addConstant`, `reportScriptError`, `getScriptProcessor`)

## obtainedVia

Created via `Engine.createFixObjectFactory(layoutDescription)`:

```cpp
// ScriptingApi.cpp:2169
var ScriptingApi::Engine::createFixObjectFactory(var layoutData)
{
    return var(new fixobj::Factory(getScriptProcessor(), layoutData));
}
```

The `layoutData` parameter is a JSON object where each property defines a member: property name becomes the member ID, property value determines the data type and default value.

## Constructor Analysis

```cpp
Factory::Factory(ProcessorWithScriptingContent* s, const var& d) :
    ConstScriptingObject(s, 0),  // 0 constants from ConstScriptingObject
    customCompareFunction(getScriptProcessor(), this, var(), 2)  // 2-arg callback holder
{
    allocator = new Allocator();

    ADD_API_METHOD_0(create);
    ADD_API_METHOD_1(createArray);
    ADD_API_METHOD_1(createStack);
    ADD_API_METHOD_1(setCompareFunction);

    addConstant("prototype", d);         // stores the original layout description
    layout = createLayout(allocator, d, &initResult);  // parses JSON into MemoryLayoutItem::List
    typeHash = Helpers::createHash(layout);  // hash based on member IDs + types
    compareFunction = BIND_MEMBER_FUNCTION_2(Factory::compare);  // default comparator
}
```

### Key observations:
- `ConstScriptingObject(s, 0)` -- zero pre-allocated constant slots (the `prototype` constant is added dynamically)
- All methods use `ADD_API_METHOD_N` (not typed variants) -- no forced types
- `initResult` captures layout parsing errors; checked by `create()`/`createArray()`/`createStack()` before proceeding
- Default compare function is the member function `Factory::compare` which uses `WeakCallbackHolder` or pointer-based fallback

## Wrapper Struct (API Registration)

```cpp
struct Factory::Wrapper
{
    API_METHOD_WRAPPER_0(Factory, create);
    API_METHOD_WRAPPER_1(Factory, createArray);
    API_METHOD_WRAPPER_1(Factory, createStack);
    API_VOID_METHOD_WRAPPER_1(Factory, setCompareFunction);
};
```

All use untyped `API_METHOD_WRAPPER_N` / `API_VOID_METHOD_WRAPPER_N`. No `ADD_TYPED_API_METHOD_N` anywhere.

## Constants

| Name | Source | Description |
|------|--------|-------------|
| `prototype` | `addConstant("prototype", d)` | The original JSON layout description passed to the constructor |

This is the only constant. It preserves the layout description so scripts can inspect it.

## LayoutBase::DataType Enum

```cpp
enum class DataType
{
    Integer,   // maps to C++ int (4 bytes)
    Boolean,   // maps to C++ int (4 bytes) -- stored as int, read as bool
    Float,     // maps to C++ float (4 bytes)
    numTypes   // sentinel
};
```

### Type inference from JSON prototype values

`Helpers::getTypeFromVar()`:
- `var.isInt()` or `var.isInt64()` -> `DataType::Integer`
- `var.isDouble()` -> `DataType::Float`
- `var.isBool()` -> `DataType::Boolean`
- `var.isArray()` -> recurse into first element to get type (array members use a single type for all elements)
- `var.isObject()` or `var.isString()` -> `Result::fail("illegal type")` -- strings and objects are NOT supported

### Array-typed members

If a prototype property value is a JSON array, `getElementSizeFromVar()` returns the array length, and the member stores that many contiguous elements of the inferred type. This enables fixed-size sub-arrays within each object (e.g., `{ "position": [0.0, 0.0, 0.0] }` creates a 3-element float array member).

## Memory Layout System (LayoutBase)

### MemoryLayoutItem

Each member in the prototype becomes a `MemoryLayoutItem`:
- `id` -- Identifier from the JSON property name
- `type` -- DataType inferred from value
- `offset` -- byte offset within the element (accumulated sequentially)
- `elementSize` -- 1 for scalars, N for array-typed members
- `defaultValue` -- the original prototype value (used for reset)

### createLayout()

```cpp
MemoryLayoutItem::List LayoutBase::createLayout(Allocator::Ptr allocator, var layoutDescription, Result* r)
{
    // Iterates DynamicObject properties in order
    // Each property creates a MemoryLayoutItem with accumulated byte offset
    // Empty layout -> Result::fail("No data")
}
```

Property order in the JSON prototype determines memory layout order. This is important because the type hash factors in both IDs and types in order.

### Type Hash

```cpp
int Helpers::createHash(MemoryLayoutItem::List list)
{
    String s;
    for(auto l: list)
    {
        s << l->id;
        s << (uint8)l->type;
    }
    return s.hashCode();
}
```

Hash is computed from concatenated member IDs + type bytes. Used for type compatibility checking. Exposed via `getTypeHash()`.

## Allocator

Custom block allocator with 16-byte alignment:
- `allocate(numBytes)` -- creates a new `HeapBlock<uint8>` with padding for 16-byte alignment
- `validMemoryAccess(ptr)` -- checks if pointer falls within any allocated block
- Assertion: `numBytesToAllocate % 4 == 0` (all types are 4 bytes)

The allocator is shared between the Factory and all objects/arrays/stacks it creates. The Factory owns it via `Allocator::Ptr` (reference counted).

## Method: create()

```cpp
var Factory::create()
{
    if (initResult.wasOk())
    {
        auto b = allocator->allocate((int)getElementSizeInBytes());
        auto r = new ObjectReference();
        r->init(this, b, true);  // resetToDefault=true
        singleObjects.add(r);    // Factory retains ownership
        return var(r);
    }
    return var();
}
```

Returns a single `ObjectReference` (script name: `FixObject`). The Factory keeps a reference in `singleObjects`.

## Method: createArray(int numElements)

```cpp
var Factory::createArray(int numElements)
{
    if (initResult.wasOk())
    {
        auto newElement = new Array(getScriptProcessor(), numElements);
        newElement->compareFunction = compareFunction;  // inherits current compare
        newElement->init(this);
        arrays.add(newElement);  // Factory retains ownership
        return var(newElement);
    }
    return {};
}
```

Creates a `fixobj::Array` (script name: `FixObjectArray`). The Array allocates a contiguous block for all elements and creates ObjectReference items pointing into it.

## Method: createStack(int numElements)

```cpp
var Factory::createStack(int numElements)
{
    if (initResult.wasOk())
    {
        auto newElement = new Stack(getScriptProcessor(), numElements);
        newElement->compareFunction = compareFunction;
        newElement->init(this);
        arrays.add(newElement);  // stored in same arrays list as Array
        return var(newElement);
    }
    return var();
}
```

Creates a `fixobj::Stack` (script name: `FixObjectStack`). Stack extends Array, adding insert/remove semantics with a position pointer.

## Method: setCompareFunction(var newCompareFunction)

This is the most complex method. It accepts three input modes:

### Mode 1: Single property name (String without comma)

```cpp
if(newCompareFunction.isString() && !text.contains(","))
{
    // Looks up property in layout by Identifier
    // Creates a NumberComparator<T, IsArray> templated on the property's DataType
    // Supports bool, int, float, with array variants
}
```

Creates an optimized C++ comparator that directly reads memory at the property's offset. No script callback overhead.

### Mode 2: Comma-separated property names (String with comma)

```cpp
if(text.contains(","))
{
    // Tokenizes by comma
    // Looks up each property in layout
    // Creates MultiComparator<N> where N = number of properties (2, 3, or 4)
    // Compares properties in order (first difference wins)
    // Error if N > 4: "At this point you might want to use a custom function"
    // Error if N < 2: "Redundant comma"
}
```

Multi-property comparison with priority order. Limited to 2-4 properties.

### Mode 3: JavaScript function

```cpp
else if (HiseJavascriptEngine::isJavascriptFunction(newCompareFunction))
{
    customCompareFunction = WeakCallbackHolder(getScriptProcessor(), this, newCompareFunction, 2);
    customCompareFunction.incRefCount();
}
```

Stores as a `WeakCallbackHolder` with 2 arguments. Called synchronously via `callSync()` in `Factory::compare()`.

### Mode 4: Reset (anything else)

```cpp
else
{
    compareFunction = BIND_MEMBER_FUNCTION_2(Factory::compare);
}
```

Resets to the default comparator (which uses `operator==` for equality, pointer address for ordering).

### Propagation to existing containers

After setting the compare function, it propagates to ALL previously created arrays/stacks:

```cpp
for(auto& a: arrays)
{
    a->compareFunction = compareFunction;
}
```

Note: `singleObjects` are NOT updated (single objects don't use comparison).

## Method: getTypeHash()

Inline in header:
```cpp
int getTypeHash() const { return typeHash; }
```

Returns the hash computed during construction from member IDs + types.

## Default Compare Function: Factory::compare()

```cpp
int Factory::compare(ObjectReference::Ptr v1, ObjectReference::Ptr v2)
{
    if (customCompareFunction)
    {
        // Calls JavaScript function synchronously with two object args
        var args[2] = { var(v1.get()), var(v2.get()) };
        var r(0);
        auto ok = customCompareFunction.callSync(args, 2, &r);
        return (int)r;
    }
    else
    {
        // Byte-level equality check, then pointer-based ordering
        if (*v1 == *v2) return 0;
        else return (p1 > p2) ? 1 : -1;  // pointer comparison
    }
}
```

The default (no custom compare) uses `ObjectReference::operator==` which does a byte-level comparison of the entire element data.

## ObjectReference Comparison (operator==)

```cpp
bool ObjectReference::operator==(const ObjectReference& other) const
{
    if (data == other.data) return true;  // same memory
    if (layout[0] == other.layout[0])     // same layout type
    {
        // Compare all bytes as ints
        auto numIntsToCheck = elementSize / sizeof(int);
        for (int i = 0; i < numIntsToCheck; i++)
            same &= (i1[i] == i2[i]);
        return same;
    }
    return false;
}
```

Full byte-level equality. Only compares objects with the same layout.

## Comparator Templates

### NumberComparator<T, IsArray>

Template that compares a single property by direct memory read at the property's byte offset:
- `T` is the C++ type (`bool`, `int`, `float`)
- `IsArray` enables element-by-element comparison for array-typed members
- Returns -1, 0, or 1

### MultiComparator<NumItems>

Fixed-size array of `Item` structs (offset, type, elementSize). Compares properties in order, returning on first difference. Template parameter `NumItems` determines the array size (2, 3, or 4 supported).

## Ownership Model

The Factory retains references to all objects it creates:
- `singleObjects` -- `ReferenceCountedArray<ObjectReference>` for `create()` results
- `arrays` -- `ReferenceCountedArray<Array>` for both `createArray()` and `createStack()` results

This prevents garbage collection of the underlying memory while the Factory exists. The Allocator (also reference-counted) is shared across the Factory and all its children.

## Threading / Lifecycle

- No explicit thread safety mechanisms (no locks, no audio-thread guards)
- `WeakCallbackHolder::callSync()` for custom compare functions -- this is synchronous
- The `setCompareFunction` propagates to existing containers immediately
- No onInit-only restrictions detected
- The data model is designed for contiguous memory access patterns suitable for real-time use (when using the optimized string-based comparators rather than JS callbacks)

## Preprocessor Guards

None. The fixobj namespace has no conditional compilation.

## Related Classes Created

### fixobj::Array (FixObjectArray)
- Extends LayoutBase + AssignableObject + ConstScriptingObject
- Constructor takes `(ProcessorWithScriptingContent*, int numElements)`
- Has constant: `length` (the numElements)
- `init(LayoutBase* parent)` copies layout from parent, allocates contiguous memory, creates ObjectReference items
- Supports bracket indexing via AssignableObject
- Methods: fill, clear, indexOf, contains, copy, toBase64, fromBase64, size, sort

### fixobj::Stack (FixObjectStack)
- Extends fixobj::Array
- Adds `position` pointer tracking active element count
- Methods: insert, remove, removeElement, size (overrides to return position), clear, clearQuick, isEmpty, set
- Has a debug Viewer component (createPopupComponent) with live updating display
- `insert` checks for duplicates via indexOf before inserting
- `removeElement` moves the last active element into the gap (swap-and-pop pattern)

### fixobj::ObjectReference (FixObject)
- Extends LayoutBase + ReferenceCountedObject + ObjectWithJSONConverter + DebugableObjectBase
- Contains `MemberReference` inner class for property access
- MemberReference supports bracket indexing for array-typed members
- JSON serialization via writeAsJSON
- Assignment operator does byte-level memcpy for same-layout objects
- Implements `JUCE_MAKE_STREAMABLE_OBJECT(4)` (stream support stubbed out with jassertfalse)
