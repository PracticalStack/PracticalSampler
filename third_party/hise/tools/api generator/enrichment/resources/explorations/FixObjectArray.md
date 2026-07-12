# FixObjectArray -- C++ Source Exploration

## Resources Consulted

- `enrichment/resources/survey/class_survey_data.json` -- FixObjectArray entry
- `enrichment/phase1/FixObjectFactory/Readme.md` -- prerequisite class (factory system architecture)
- `HISE/hi_scripting/scripting/api/FixLayoutObjects.h` -- class declaration
- `HISE/hi_scripting/scripting/api/FixLayoutObjects.cpp` -- full implementation
- `HISE/hi_scripting/scripting/engine/JavascriptEngineStatements.cpp` -- for-in loop integration

## Prerequisite Context (FixObjectFactory)

FixObjectFactory defines a typed memory schema from a JSON prototype and creates containers (FixObjectArray, FixObjectStack) and single FixObject instances. The factory owns a shared Allocator (16-byte aligned), manages comparison functions, and propagates them to all created containers. Supported data types are Integer, Float, Boolean, and fixed-size arrays of these. The memory layout is immutable after construction.

FixObjectArray is one of the two container types produced by FixObjectFactory. It represents a fixed-size array where all slots are always valid (contrast with FixObjectStack's variable occupancy). The factory creates it via `Factory::createArray(int numElements)`.

---

## Class Declaration

**File:** `HISE/hi_scripting/scripting/api/FixLayoutObjects.h`, lines 336-419

```cpp
struct Array : public LayoutBase,
    public AssignableObject,
    public ObjectWithJSONConverter,
    public ConstScriptingObject
{
    ObjectReference::CompareFunction compareFunction;
    struct Wrapper;
    // ...
protected:
    size_t elementSize = 0;
    size_t numElements = 0;
    size_t numAllocated = 0;
    ReferenceCountedArray<ObjectReference> items;
    uint8* data;
};
```

### Inheritance Chain

| Base Class | Purpose |
|------------|---------|
| `LayoutBase` | Provides `layout` (MemoryLayoutItem::List), `allocator` (Allocator::Ptr), `typeHash`, `getElementSizeInBytes()` |
| `AssignableObject` | Enables bracket-index access (`arr[i]`) via `assign()`, `getAssignedValue()`, `getCachedIndex()` |
| `ObjectWithJSONConverter` | Enables JSON serialization via `writeAsJSON()`, `writeToStream()`, `createFromStream()` |
| `ConstScriptingObject` | Scripting API base: constant registration, method registration, `reportScriptError()` |

### Key Fields

| Field | Type | Description |
|-------|------|-------------|
| `compareFunction` | `ObjectReference::CompareFunction` (= `std::function<int(Ptr, Ptr)>`) | Comparison function shared from the factory. Used by `indexOf`, `contains`, `sort`. |
| `elementSize` | `size_t` | Byte size of a single element (computed from layout) |
| `numElements` | `size_t` | Fixed number of elements (set at construction, never changes) |
| `numAllocated` | `size_t` | Total bytes allocated (`elementSize * numElements`) |
| `items` | `ReferenceCountedArray<ObjectReference>` | Array of ObjectReference wrappers, one per element slot |
| `data` | `uint8*` | Raw contiguous memory block from the allocator |

---

## Constructor and Initialization

**Constructor** (`FixLayoutObjects.cpp`, line 856):

```cpp
Array::Array(ProcessorWithScriptingContent* s, int numElements):
    ConstScriptingObject(s, 1)  // 1 constant slot
{
    registerStreamCreator(this);
    addConstant("length", numElements);

    ADD_API_METHOD_1(indexOf);
    ADD_API_METHOD_1(contains);
    ADD_API_METHOD_1(fill);
    ADD_API_METHOD_0(clear);
    ADD_API_METHOD_2(copy);
    ADD_API_METHOD_0(size);
    ADD_API_METHOD_0(sort);
    ADD_API_METHOD_0(toBase64);
    ADD_API_METHOD_1(fromBase64);
}
```

**Constants registered:** 1

| Name | Value | Type | Description |
|------|-------|------|-------------|
| `length` | constructor arg | Integer | Fixed number of elements in the array |

**Note:** The constructor passes `1` to `ConstScriptingObject` (numConstants = 1), matching the single `addConstant("length", ...)` call.

**Initialization** (`init()`, line 922):

The `init(LayoutBase* parent)` method is called by the factory after construction:

1. Copies `layout` and `allocator` from parent factory
2. Reads `numElements` from constant slot 0 (`getConstantValue(0)`)
3. Computes `elementSize` and `numAllocated`
4. Allocates contiguous memory block via `allocator->allocate()`
5. Creates `numElements` ObjectReference instances, each initialized with a pointer into the contiguous block
6. Each ObjectReference is initialized with `resetToDefault = true`

The contiguous memory layout means elements are packed sequentially: element i starts at `data + i * elementSize`.

---

## API Method Registrations

All 9 methods use plain `ADD_API_METHOD_N` (no typed variants):

| Method | Wrapper Macro | Params |
|--------|---------------|--------|
| `indexOf` | `API_METHOD_WRAPPER_1` | 1 |
| `fill` | `API_VOID_METHOD_WRAPPER_1` | 1 |
| `clear` | `API_VOID_METHOD_WRAPPER_0` | 0 |
| `contains` | `API_METHOD_WRAPPER_1` | 1 |
| `copy` | `API_METHOD_WRAPPER_2` | 2 |
| `sort` | `API_VOID_METHOD_WRAPPER_0` | 0 |
| `size` | `API_METHOD_WRAPPER_0` | 0 |
| `toBase64` | `API_METHOD_WRAPPER_0` | 0 |
| `fromBase64` | `API_METHOD_WRAPPER_1` | 1 |

**No `ADD_TYPED_API_METHOD_N` registrations exist for this class.** All parameter types must be inferred from the C++ signatures.

---

## Factory Creation Pattern

**In `Factory::createArray()` (line 347):**

```cpp
var Factory::createArray(int numElements)
{
    if (initResult.wasOk())
    {
        auto newElement = new Array(getScriptProcessor(), numElements);
        newElement->compareFunction = compareFunction;
        newElement->init(this);
        arrays.add(newElement);
        return var(newElement);
    }
    return {};
}
```

Key points:
- The factory's current `compareFunction` is copied to the new array at creation time
- The factory retains a reference via `arrays.add(newElement)`
- If `Factory::setCompareFunction()` is called later, it updates all previously created arrays (line 472-475 in the factory code)

---

## AssignableObject Interface (Bracket Indexing)

The `AssignableObject` interface enables `arr[i] = obj` and `var x = arr[i]` syntax.

**`assign(int index, var newValue)`** (line 873):
- Expects `newValue` to be an `ObjectReference*` (via `dynamic_cast`)
- Copies the source object's data into the target slot: `*items[index] = *fo`
- This is a deep copy of the raw memory (via ObjectReference's `operator=` which does `memcpy`)

**`getAssignedValue(int index)`** (line 884):
- Returns `var(items[index].get())` -- a reference to the ObjectReference at that index
- Bounds-checked via `isPositiveAndBelow`

**`getCachedIndex(const var& indexExpression)`** (line 894):
- Simply casts to int: `return (int)indexExpression`

---

## For-In Loop Integration

**File:** `JavascriptEngineStatements.cpp`, lines 1205-1264

FixObjectArray is directly supported in HiseScript's `for (x in arr)` loop construct:

### Size determination (line 1263):
```cpp
else if (auto fixArray = dynamic_cast<fixobj::Array*>(currentObject.getObject()))
    size = fixArray->getConstantValue(0);  // reads "length" constant
```

For `fixobj::Array` (not Stack), the loop iterates over ALL elements (uses the `length` constant). For `fixobj::Stack`, it uses `size()` (which returns `position`, the number of occupied slots).

### Element access (line 1205):
```cpp
else if (auto fo = dynamic_cast<fixobj::Array*>(data->getObject()))
    return fo->getAssignedValue(loop->index);
```

### Element assignment in loop (line 1223):
```cpp
else if (auto fo = dynamic_cast<fixobj::Array*>(data->getObject()))
{
    auto v = dynamic_cast<fixobj::ObjectReference*>(fo->getAssignedValue(loop->index).getObject());
    auto s = dynamic_cast<fixobj::ObjectReference*>(newValue.getObject());
    *v = *s;
}
```

This means elements accessed via for-in are live references -- modifying them modifies the array's underlying memory.

---

## Compare Function Dependency

The `compareFunction` field is central to three methods: `indexOf`, `contains`, and `sort`.

### How indexOf works (line 974):
- Linear search through `size()` elements
- Uses `compareFunction(item, o) == 0` for equality
- Returns -1 if not found or if the argument is not an ObjectReference

### How contains works (line 1119):
- Simply delegates to `indexOf(obj) != -1`

### How sort works (line 1073):
- Creates a local `Sorter` struct that delegates to `compareFunction`
- Uses `std::sort` with a `SortFunctionConverter` adapter
- Sorts only up to `size()` elements (relevant for Stack subclass)

### Default compare behavior:
When no custom compare function is set on the factory, the default `Factory::compare()` method (line 478) is used:
1. First checks byte-level equality via `ObjectReference::operator==` (memcmp-style)
2. If not equal, falls back to pointer address ordering

---

## Method Implementation Details

### `fill(var obj)` (line 955)
- If `obj` is an ObjectReference: copies it into every slot via `*i = *o` (deep copy)
- If `obj` is anything else (including undefined): calls `i->clear()` on every slot, resetting to defaults

### `clear()` (line 969)
- Delegates to `fill(var())` -- resets all elements to their default values

### `copy(String propertyName, var target)` (line 995)
- Looks up the property by name in the layout to get its offset and type
- Reports script error if property name is not found
- If target is a Buffer: checks size match, copies each element's property value as float
- If target is an Array: uses `ensureStorageAllocated` and `set()` to populate
- Returns false if target is neither Buffer nor Array
- Reads directly from raw memory using the property offset, stepping by elementSize per element

### `toBase64()` (line 1048)
- Creates a `MemoryBlock` from the raw `data` pointer with `numAllocated` size
- Returns `mb.toBase64Encoding()`

### `fromBase64(const String& b64)` (line 1054)
- Decodes the base64 string into a MemoryBlock
- Validates that decoded size matches `numAllocated` exactly
- If match: `memcpy` into `data`; returns true
- If mismatch: returns false (no error thrown, silent failure)

### `size()` (line 1068)
- Returns `(int)numElements` -- always the fixed capacity
- Note: This is `virtual` and overridden by Stack to return `position` instead

---

## ObjectReference -- Element Type

Each element in the array is an `ObjectReference` instance (declared at line 134 of the header).

Key characteristics:
- Holds a raw `uint8* data` pointer into the contiguous memory block
- Has a `NamedValueSet memberReferences` for named property access
- `operator==` does byte-level comparison of the entire element data
- `operator=` does `memcpy` of element data (deep copy)
- `clear()` resets all members to default values
- Implements `ObjectWithJSONConverter` for JSON serialization
- Debug name: "FixObject"

---

## JSON Serialization

### `writeAsJSON()` (line 1098)
- Outputs as JSON array: `[ {...}, {...}, ... ]`
- Iterates up to `size()` elements (important: Stack uses position, Array uses numElements)
- Each element is serialized via `ObjectReference::writeAsJSON()`

### ObjectReference JSON format (line 551):
- Outputs as JSON object: `{ "prop1": value1, "prop2": value2 }`
- Float types use `Types::ID::Double` for formatting
- Integer/Boolean types use `Types::ID::Integer`

---

## Threading and Lifecycle

- No explicit thread safety mechanisms in the Array class
- No mutex, no atomic, no lock-free constructs
- The `ConstScriptingObject` base does not add thread safety
- All operations are intended for single-thread use (scripting thread)
- Memory is allocated once during `init()` and never reallocated
- The allocator is reference-counted and shared with the factory

---

## Preprocessor Guards

None. The FixObjectArray class has no conditional compilation. It is always available.

---

## Relationship to FixObjectStack

`fixobj::Stack` inherits from `fixobj::Array` (line 423 of the header). It overrides:
- `size()` -- returns `position` instead of `numElements`
- `clear()` -- resets elements AND sets position to 0

Stack adds: `insert`, `remove`, `removeElement`, `clearQuick`, `isEmpty`, `set`

The key semantic difference: Array always has all slots valid (iteration covers all elements). Stack has a variable occupancy tracked by `position`.

---

## JUCE_MAKE_STREAMABLE_OBJECT

The Array class uses `JUCE_MAKE_STREAMABLE_OBJECT(5)` (line 370), registering it with streamable object ID 5. However, `writeToStream()` and `createFromStream()` both contain `jassertfalse` -- streaming is not yet implemented. The Base64 methods provide the actual serialization mechanism.
