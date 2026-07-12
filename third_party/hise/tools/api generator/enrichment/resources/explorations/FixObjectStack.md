# FixObjectStack -- C++ Source Exploration

## Resources Consulted

- `enrichment/resources/survey/class_survey_data.json` -- FixObjectStack entry
- `enrichment/phase1/FixObjectFactory/Readme.md` -- prerequisite class analysis
- `HISE/hi_scripting/scripting/api/FixLayoutObjects.h` -- class declarations
- `HISE/hi_scripting/scripting/api/FixLayoutObjects.cpp` -- full implementation

## Prerequisite: FixObjectFactory Context

FixObjectStack operates within the FixObjectFactory system described in the prerequisite Readme. Key points:
- Factory defines a typed memory layout from a JSON prototype (int, float, bool, arrays of these)
- Factory creates containers (FixObjectArray, FixObjectStack) and single objects (FixObject)
- All containers share the factory's memory allocator (16-byte aligned) and layout definition
- Factory manages comparison functions propagated to all containers
- Supported data types: Integer (4 bytes), Float (4 bytes), Boolean (stored as int, 4 bytes), Array of numeric types

---

## Class Declaration

**File:** `HISE/hi_scripting/scripting/api/FixLayoutObjects.h`, lines 423-468

**Namespace:** `hise::fixobj`

**Scripting name:** `"FixObjectStack"` (returned by `getObjectName()`)

```cpp
struct Stack : public Array
{
    struct Viewer;
    struct Wrapper;

    Stack(ProcessorWithScriptingContent* s, int numElements);

    Identifier getObjectName() const override;
    Component* createPopupComponent(const MouseEvent& e, Component* parent) override;

    // API methods
    bool insert(var obj);
    int size() const override;
    bool remove(var obj);
    bool removeElement(int index);
    void clear() override;
    void clearQuick();
    bool isEmpty() const;
    bool set(var obj);

private:
    ObjectReference* getRef(const var& obj);
    int position = 0;
};
```

### Inheritance Chain

```
LayoutBase
  -> fixobj::Array (ConstScriptingObject, AssignableObject, ObjectWithJSONConverter)
    -> fixobj::Stack
```

Stack inherits from `fixobj::Array` which inherits from:
- `LayoutBase` -- memory layout definition, allocator reference, layout items
- `ConstScriptingObject` -- scripting API registration (ADD_API_METHOD macros)
- `AssignableObject` -- bracket-indexed element access (`stack[index]`)
- `ObjectWithJSONConverter` -- JSON serialization support

### Key Private State

- `int position = 0` -- the current end pointer; elements at indices `[0, position)` are "used"
- Inherited from Array: `items` (ReferenceCountedArray<ObjectReference>), `data` (raw memory), `numElements` (capacity), `elementSize` (bytes per element), `compareFunction`

---

## Constructor

**File:** `FixLayoutObjects.cpp`, lines 1286-1299

```cpp
Stack::Stack(ProcessorWithScriptingContent* s, int numElements):
    Array(s, numElements)
{
    ADD_API_METHOD_1(insert);
    ADD_API_METHOD_1(remove);
    ADD_API_METHOD_1(removeElement);
    ADD_API_METHOD_0(size);
    ADD_API_METHOD_1(indexOf);
    ADD_API_METHOD_1(contains);
    ADD_API_METHOD_0(isEmpty);
    ADD_API_METHOD_1(set);
    ADD_API_METHOD_0(clear);
    ADD_API_METHOD_0(clearQuick);
}
```

All methods use plain `ADD_API_METHOD_N` -- no typed variants (`ADD_TYPED_API_METHOD_N`). This means all parameter types must be inferred from the implementations.

The parent Array constructor (called first) registers: `indexOf`, `contains`, `fill`, `clear`, `copy`, `size`, `sort`, `toBase64`, `fromBase64`. Stack re-registers some of these (`indexOf`, `contains`, `size`, `clear`) which overrides the parent registration. Stack also adds its own unique methods: `insert`, `remove`, `removeElement`, `isEmpty`, `set`, `clearQuick`.

### Constants

The parent Array constructor registers one constant:
```cpp
addConstant("length", numElements);  // Array constructor, line 857
```

Stack does not add any additional constants beyond what Array provides.

---

## Factory Creation

**File:** `FixLayoutObjects.cpp`, lines 361-373

Stack instances are created by `Factory::createStack(int numElements)`:

```cpp
var Factory::createStack(int numElements)
{
    if (initResult.wasOk())
    {
        auto newElement = new Stack(getScriptProcessor(), numElements);
        newElement->compareFunction = compareFunction;
        newElement->init(this);
        arrays.add(newElement);
        return var(newElement);
    }
    return var();
}
```

Key observations:
1. The factory's current `compareFunction` is copied to the new stack
2. `init(this)` is called from Array::init, which copies layout, allocates memory, creates ObjectReference items for all slots
3. The stack is added to the factory's `arrays` list (shared with FixObjectArray instances) -- this allows `setCompareFunction` to propagate to all containers

---

## Array::init -- Memory Allocation (inherited)

**File:** `FixLayoutObjects.cpp`, lines 922-953

```cpp
void Array::init(LayoutBase* parent)
{
    layout = parent->layout;
    allocator = parent->allocator;
    numElements = (int)getConstantValue(0);  // reads "length" constant
    elementSize = getElementSizeInBytes();
    numAllocated = getElementSizeInBytes() * (numElements);
    typeHash = Helpers::createHash(layout);

    if (numAllocated > 0)
    {
        data = allocator->allocate((int)numAllocated);
        for (int i = 0; i < numElements; i++)
        {
            auto ptr = data + i * elementSize;
            auto obj = new ObjectReference();
            obj->init(this, ptr, true);
            items.add(obj);
        }
    }
}
```

All `numElements` slots are pre-allocated at creation time. Each slot gets an ObjectReference pointing into the contiguous memory block. The `position` pointer (Stack-specific) starts at 0, meaning the stack is initially empty despite all slots being allocated.

---

## Stack-Specific Method Implementations

### insert(var obj) -- lines 1312-1328

```cpp
bool Stack::insert(var obj)
{
    auto idx = indexOf(obj);
    if (idx != -1)
        return false;  // duplicate detection via compare function

    if (auto ref = getRef(obj))
    {
        *items[position] = *ref;
        position = jmin<int>(position + 1, (int)numElements - 1);
        return true;
    }
    return false;
}
```

**Behavioral notes:**
- Checks for duplicates using `indexOf()` (which uses the compare function). If found, returns false without inserting.
- Copies data from the source ObjectReference into the slot at `position`.
- Advances `position` but clamps to `numElements - 1` -- this means the stack silently overwrites the last element when full rather than failing. This is a subtle capacity behavior.
- Returns false if `obj` is not an ObjectReference (type check via `getRef()`).

### size() -- line 1330

```cpp
int Stack::size() const { return position; }
```

Overrides Array::size(). Returns the number of used slots (0 to position), not the allocated capacity. The capacity is available via the `length` constant.

### remove(var obj) -- lines 1335-1343

```cpp
bool Stack::remove(var obj)
{
    auto idx = indexOf(obj);
    if (idx != -1)
        return removeElement(idx);
    return false;
}
```

Finds the element by compare function, then delegates to `removeElement`.

### removeElement(int index) -- lines 1345-1357

```cpp
bool Stack::removeElement(int index)
{
    if (isPositiveAndBelow(index, position))
    {
        position = jmax<int>(0, position - 1);
        *items[index] = *items[position];
        items[position]->clear();
        return true;
    }
    return false;
}
```

**Behavioral notes:**
- Does NOT preserve order. Copies the last used element into the gap, then clears the last slot. This is a swap-and-pop pattern.
- Decrements `position` (clamped to 0).
- Returns false for out-of-range indices.

### clear() -- lines 1359-1365

```cpp
void Stack::clear()
{
    for (auto i : items)
        i->clear();  // resets each ObjectReference to default values
    clearQuick();
}
```

Resets all items (including unused slots beyond `position`) to their default values, then resets position to 0.

### clearQuick() -- lines 1367-1370

```cpp
void Stack::clearQuick()
{
    position = 0;
}
```

Just resets the position pointer. Elements remain in memory with their old values. Useful for performance when the data will be overwritten.

### isEmpty() -- lines 1372-1375

```cpp
bool Stack::isEmpty() const { return position == 0; }
```

### set(var obj) -- lines 1377-1398

```cpp
bool Stack::set(var obj)
{
    if(isEmpty())
        assign(position++, obj);
    else
    {
        auto idx = indexOf(obj);
        if(idx == -1)
        {
            if(isPositiveAndBelow(position, (int)numElements - 1))
                assign(position++, obj);
            else
                return false;
        }
        else
            assign(idx, obj);
    }
    return true;
}
```

**Behavioral notes:**
- If empty, inserts at position 0.
- If the object exists (by compare function), replaces it in-place.
- If the object does not exist and there is room, inserts at end.
- Returns false only when the stack is full and the object is not found.
- Uses `assign()` (inherited from Array) which calls `*items[index] = *ref` via the AssignableObject interface.

### getRef(const var& obj) -- lines 1307-1310

```cpp
ObjectReference* Stack::getRef(const var& obj)
{
    return dynamic_cast<ObjectReference*>(obj.getObject());
}
```

Simple type-check helper. Returns nullptr if `obj` is not a FixObject.

---

## Inherited Methods from Array

These methods are inherited and not overridden by Stack. They operate on all `numElements` slots (the full capacity), ignoring the `position` pointer, except where noted:

### indexOf(var obj) -- lines 974-993

```cpp
int Array::indexOf(var obj) const
{
    if (auto o = dynamic_cast<ObjectReference*>(obj.getObject()))
    {
        int numToSearch = size();  // VIRTUAL: calls Stack::size() = position
        // ...
        for (int i = 0; i < numToSearch; i++)
        {
            if (compareFunction(item, o) == 0)
                return i;
        }
    }
    return -1;
}
```

**Critical:** Uses `size()` which is virtual -- for Stack, this returns `position`, so indexOf only searches the used portion.

### contains(var obj) -- line 1119-1122

Delegates to `indexOf(obj) != -1`. Only searches used portion because indexOf uses virtual size().

### fill(var obj) -- lines 955-967

Fills ALL allocated slots (iterates `items`, not limited by position). For Stack, this fills the entire capacity, not just the used portion. Does not update `position`.

### copy(String propertyName, var target) -- lines 995-1046

Copies a named property from each element into a Buffer or Array. Uses `numElements` (full capacity), NOT `size()`. For Stack, this copies from all slots including unused ones.

### toBase64() / fromBase64(String) -- lines 1048-1066

Serializes/deserializes the entire raw memory block (`numAllocated` bytes). This includes all slots regardless of position.

### sort() -- lines 1073-1096

Sorts elements from `items.begin()` to `items.begin() + size()`. Uses virtual `size()`, so for Stack only sorts the used portion `[0, position)`.

---

## Comparison Function Integration

The compare function is critical infrastructure shared between Factory, Array, and Stack.

Stack inherits `compareFunction` from Array. It is set by the factory at creation time and updated when `Factory::setCompareFunction()` is called (which iterates all created arrays/stacks).

Methods that use the compare function:
- `indexOf()` (inherited) -- equality check (`compareFunction(item, o) == 0`)
- `sort()` (inherited) -- ordering
- `insert()` -- duplicate detection via `indexOf()`
- `remove()` -- element lookup via `indexOf()`
- `set()` -- existence check via `indexOf()`

With the default compare function (byte-level equality + pointer ordering), two objects are "equal" only if all their bytes match. Setting a property-based comparator changes the semantics -- objects are equal if the specified properties match, even if other properties differ.

---

## Debug Viewer

**File:** `FixLayoutObjects.cpp`, lines 1124-1270

`Stack::Viewer` is a JUCE Component subclass that provides a visual debug view:
- Table layout with columns per layout property and rows per element
- Shows used rows distinctly from unused rows (`r->used = i < obj->position`)
- Highlights recently changed values with a fading alpha animation
- Updates via `PooledUIUpdater::SimpleTimer` (UI thread)
- Created via `createPopupComponent()` for the HISE debugger

The viewer displays up to 16 rows (`jmin<int>(16, numElements)`), with each column 100px wide.

---

## Wrapper Struct (API Registration)

**File:** `FixLayoutObjects.cpp`, lines 1272-1284

```cpp
struct Stack::Wrapper
{
    API_METHOD_WRAPPER_1(Stack, insert);
    API_METHOD_WRAPPER_1(Stack, remove);
    API_METHOD_WRAPPER_1(Stack, removeElement);
    API_METHOD_WRAPPER_0(Stack, size);
    API_METHOD_WRAPPER_1(Stack, indexOf);
    API_METHOD_WRAPPER_1(Stack, contains);
    API_METHOD_WRAPPER_0(Stack, isEmpty);
    API_VOID_METHOD_WRAPPER_1(Stack, set);
    API_VOID_METHOD_WRAPPER_0(Stack, clear);
    API_VOID_METHOD_WRAPPER_0(Stack, clearQuick);
};
```

All use plain `API_METHOD_WRAPPER_N` / `API_VOID_METHOD_WRAPPER_N` -- no typed variants.

Note: The Wrapper lists `set` as `API_VOID_METHOD_WRAPPER_1` but the actual method returns `bool`. This is a minor discrepancy in the wrapper -- the return value is discarded at the wrapper level.

---

## Threading / Lifecycle

- No thread safety mechanisms in Stack or Array. No locks, no atomic operations.
- The allocator uses standard heap allocation (not lock-free).
- Stack is designed for use on a single thread (typically the scripting/message thread).
- No onInit-only restrictions observed -- stack can be created and used at any time after factory creation.

---

## Preprocessor Guards

None. The fixobj namespace has no conditional compilation guards.

---

## Capacity Behavior Summary

| Operation | At capacity behavior |
|-----------|---------------------|
| `insert()` | Overwrites last element (clamps position to numElements-1) |
| `set()` | Returns false if object not found and stack is full |
| `remove()` / `removeElement()` | Swap-and-pop, does not preserve order |
| `clearQuick()` | Resets position only, data persists in memory |
| `clear()` | Resets all elements to defaults AND resets position |

---

## Key Semantic Differences: Stack vs Array

| Aspect | FixObjectArray | FixObjectStack |
|--------|---------------|----------------|
| Scripting name | `"FixObjectArray"` | `"FixObjectStack"` |
| size() | Returns total capacity (numElements) | Returns used count (position) |
| Has insert/remove | No | Yes |
| Has set | No | Yes |
| Has isEmpty/clearQuick | No | Yes |
| indexOf/contains scope | Searches all elements | Searches only used elements (via virtual size()) |
| sort scope | Sorts all elements | Sorts only used elements |
| fill scope | All elements | All elements (ignores position) |
| copy scope | All elements (numElements) | All elements (numElements, ignores position) |
| toBase64/fromBase64 | Full memory block | Full memory block (includes unused slots) |
