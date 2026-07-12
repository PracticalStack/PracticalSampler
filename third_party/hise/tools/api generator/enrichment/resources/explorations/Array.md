# Array -- C++ Source Exploration

## Resources Consulted
- `enrichment/resources/survey/class_survey_data.json` -- Array entry
- `enrichment/base/Array.json` -- 27 methods
- No prerequisites (Array is a built-in JavaScript engine type)

## Class Declaration

**File:** `HISE/hi_scripting/scripting/engine/JavascriptEngineObjects.cpp`, lines 42-671

```cpp
struct HiseJavascriptEngine::RootObject::ArrayClass : public DynamicObject
```

Array is NOT an `ApiClass` subclass. It extends `juce::DynamicObject` directly. This means:
- No `addConstant()` calls (no constants)
- No `ADD_API_METHOD_N` / `ADD_TYPED_API_METHOD_N` registrations
- Methods are registered via `setMethod(name, nativeFunction)` (JUCE's DynamicObject mechanism)
- No forced parameter types -- all methods use untyped `var` arguments

The class is a prototype-based "native object" registered with the engine at construction time. All array instances share these methods through the JUCE prototype chain lookup.

## Registration

**File:** `HISE/hi_scripting/scripting/engine/JavascriptEngineAdditionalMethods.cpp`, line 164

```cpp
registerNativeObject(RootObject::ArrayClass::getClassName(), new RootObject::ArrayClass());
```

This is called during `HiseJavascriptEngine` construction, alongside ObjectClass, StringClass, MathClass, JSONClass, and IntegerClass. These are the built-in types available in every HiseScript context.

## Constructor -- Method Registration

```cpp
ArrayClass()
{
    setMethod("contains", contains);
    setMethod("remove", remove);
    setMethod("removeElement", removeElement);
    setMethod("join", join);
    setMethod("push", push);
    setMethod("pushIfNotAlreadyThere", pushIfNotAlreadyThere);
    setMethod("pop", pop);
    setMethod("shift", shift);
    setMethod("sortNatural", sortNatural);
    setMethod("insert", insert);
    setMethod("concat", concat);
    setMethod("indexOf", indexOf);
    setMethod("isArray", isArray);
    setMethod("reverse", reverse);
    setMethod("reserve", reserve);
    setMethod("clear", clear);

    // JS-compatible aliases and additions
    setMethod("includes", contains);       // alias for contains
    setMethod("lastIndexOf", lastIndexOf);
    setMethod("isEmpty", isEmpty);
    setMethod("slice", slice);
    setMethod("toString", toStringMethod);
}
```

Key observations:
- `includes` is a direct alias to the same `contains` function pointer
- `toString` is registered but NOT in the base JSON (internal method)
- `clone` is NOT registered in ArrayClass -- it is inherited from ObjectClass (`cloneFn` calls `a.thisObject.clone()`, which is `juce::var::clone()`)

## Scoped Functions (Special Dispatch)

Eight methods require access to the JavaScript `Scope` for executing callback functions. These use a special dispatch mechanism:

```cpp
typedef var(*ScopedNativeFunction)(Args, const Scope&);

static ScopedNativeFunction getScopedFunction(const Identifier& id)
```

The scoped functions are: `find`, `findIndex`, `some`, `map`, `filter`, `forEach`, `every`, `sort`.

These are NOT registered via `setMethod()` for their scoped variant. Instead, `FunctionCall::getResult()` in `JavascriptEngineAdditionalMethods.cpp` (line 417) checks:

```cpp
if (thisObject.isArray())
{
    if (auto sf = ArrayClass::getScopedFunction(dot->child))
    {
        // ... direct invocation with scope
        return sf(args, s);
    }
}
```

This means scoped functions bypass the normal prototype lookup when called on arrays. They are invoked with the current `Scope` object, which allows them to call user-provided callback functions.

The non-scoped path (lines 478-480 in `findFunctionCall`) also catches array methods:

```cpp
if (targetObject.isArray())
    if (var* m = findRootClassProperty(ArrayClass::getClassName(), functionName))
        return *m;
```

## Method Resolution Order

For any `array.method()` call:
1. Check if it's a `DynamicObject` method (custom properties on the array itself)
2. Check scoped functions (`find`, `findIndex`, `some`, `map`, `filter`, `forEach`, `every`, `sort`)
3. Check ArrayClass registered methods (`contains`, `push`, etc.)
4. Check ObjectClass methods (`clone`, `dump`, `keys`, `toString`)
5. Throw "Unknown function"

## The `length` Property

Array `.length` is NOT a method -- it is handled as a special case in the DotIdentifier expression evaluator:

**File:** `JavascriptEngineExpressions.cpp`, lines 276-278

```cpp
if (child == DotIds::length)
{
    if (Array<var>* array = p.getArray())   return array->size();
    if (p.isBuffer()) return p.getBuffer()->size;
    if (p.isString())                       return p.toString().length();
}
```

This is a read-only pseudo-property. It cannot be set.

## Internal Helper: callForEach

All scoped functional methods (find, findIndex, some, map, filter, forEach, every) share a common iteration engine:

```cpp
using ReturnFunction = std::function<bool(int index, const var& functionReturnValue,
                                          const var& elementValue, var* totalReturnValue)>;

static var callForEach(Args a, const Scope& parent, const ReturnFunction& rf)
```

The pattern:
1. Gets the callback function from `get(a, 0)`
2. Gets optional `thisObject` from `get(a, 1)`
3. Iterates over the array, skipping `undefined`/`void` elements
4. For each element, calls the user function with `(element, index, array)` arguments
5. The ReturnFunction decides how to accumulate results and when to break

The callback receives up to 3 arguments depending on its parameter count (determined by `getNumArgs`):
- `arg[0]` = element value
- `arg[1]` = index
- `arg[2]` = the array itself

### Callback Function Types

The `isFunctionObject` helper accepts three types:
- `FunctionObject` -- regular `function() {}` declarations
- `InlineFunction::Object` -- `inline function name() {}` declarations
- Native method functions (`.isMethod()`)

## Internal Helper: isFunctionObject / getNumArgs

```cpp
static bool isFunctionObject(const var& f)
{
    if (dynamic_cast<FunctionObject*>(f.getObject())) return true;
    if (dynamic_cast<InlineFunction::Object*>(f.getObject())) return true;
    if (f.isMethod()) return true;
    return false;
}

static int getNumArgs(const var& f)
{
    if (auto fo = dynamic_cast<FunctionObject*>(f.getObject()))
        return fo->parameters.size();
    if (auto ilf = dynamic_cast<InlineFunction::Object*>(f.getObject()))
        return ilf->parameterNames.size();
    if (f.isMethod())
        return 1;
    return 0;
}
```

The number of arguments determines how many of `(element, index, array)` the callback actually receives via `NativeFunctionArgs`.

## VariantComparator (Default Sort)

**File:** `JavascriptApiClass.h`, lines 17-45

Used by `sort()` when no custom comparator is provided:

```cpp
struct VariantComparator
{
    int compareElements(const var &a, const var &b) const
    {
        if (isNumericOrUndefined(a) && isNumericOrUndefined(b))
            return (a.isDouble() || b.isDouble())
                ? returnCompareResult<double>(a, b)
                : returnCompareResult<int>(a, b);

        if ((a.isUndefined() || a.isVoid()) && (b.isUndefined() || b.isVoid()))
            return 0;

        if (a.isArray() || a.isObject())
            throw String("Can't compare arrays or objects");

        return 0;
    };
};
```

Key behaviors:
- Numeric types (int, double, int64, bool) are sorted numerically
- Mixed int/double promotes to double comparison
- undefined/void elements compare as equal
- Arrays and objects throw an exception
- Strings: the comparator returns 0 for non-numeric, non-undefined, non-array types -- so strings all compare equal (effectively unsorted)

## Custom Sort (with Comparator Function)

When `sort()` receives a function argument:
- Uses `std::stable_sort` (preserves relative order of equal elements)
- The comparator function receives two elements and should return negative/zero/positive
- Element is "less than" if comparator returns < 0
- Supports both `FunctionObject` and `InlineFunction::Object`

```cpp
std::stable_sort(array->begin(), array->end(), comparator);
```

## sortNatural

Uses `juce::String::compareNatural` which handles embedded numbers:

```cpp
std::sort(array->begin(), array->end(),
    [](const String& a, const String& b) { return a.compareNatural(b) < 0; });
```

This converts all elements to String via implicit `var` -> `String` conversion, then uses natural sort order (e.g., "item2" < "item10").

## Audio Thread Safety

### Methods with WARN_IF_AUDIO_THREAD guards:

1. **`join`** -- `WARN_IF_AUDIO_THREAD(true, IllegalAudioThreadOps::StringCreation)` -- always warns on audio thread (string allocation)
2. **`push`** -- `WARN_IF_AUDIO_THREAD(a.numArguments + array->size() >= array->getNumAllocated(), ScriptGuard::ArrayResizing)` -- warns only when pushing would cause reallocation
3. **`pushIfNotAlreadyThere`** -- same guard as `push`

### Expression-level guards (not in ArrayClass but affect array usage):

- Array subscript assignment beyond allocated size: `WARN_IF_AUDIO_THREAD(i >= ar->getNumAllocated(), ScriptAudioThreadGuard::ArrayResizing)` (JavascriptEngineExpressions.cpp:196)
- Array literal creation with non-empty values: `WARN_IF_AUDIO_THREAD(!values.isEmpty(), ScriptAudioThreadGuard::ArrayCreation)` (JavascriptEngineExpressions.cpp:575)

### Safe on audio thread (no guards):
- `contains`, `indexOf`, `lastIndexOf`, `isEmpty` -- read-only
- `pop`, `shift` -- shrinks array, no allocation
- `remove`, `removeElement` -- shrinks array
- `clear` -- uses `clearQuick()`, no allocation
- `reverse` -- creates a temporary array (potential allocation, but no guard)
- `reserve` -- explicitly allocates (should typically be called in onInit, not audio thread)

### Audio thread pattern:
Use `reserve()` in onInit to pre-allocate, then `push`/`pop` on the audio thread without triggering reallocation warnings.

## indexOf -- typeStrictness Parameter

```cpp
static var indexOf(Args a)
{
    const int typeStrictness = getInt(a, 2);
    const var target(get(a, 0));

    for (int i = (a.numArguments > 1 ? getInt(a, 1) : 0); i < array->size(); ++i)
    {
        if (typeStrictness)
            // Uses equalsWithSameType -- both value AND type must match
            if (array->getReference(i).equalsWithSameType(target))
                return i;
        else
            // Uses operator== -- loose comparison (e.g., 1 == 1.0 is true)
            if (array->getReference(i) == target)
                return i;
    }
    return -1;
}
```

Parameters: `(elementToLookFor, startOffset=0, typeStrictness=0)`
- `typeStrictness=0` (default): loose comparison via `var::operator==`
- `typeStrictness=1`: strict comparison via `var::equalsWithSameType` (type AND value must match)

## clone (inherited from ObjectClass)

The `clone` method is on ObjectClass, not ArrayClass. It calls `juce::var::clone()`:

```cpp
static var cloneFn(Args a) { return a.thisObject.clone(); }
```

`juce::var::clone()` performs a deep copy for arrays -- it creates a new `Array<var>` and recursively clones each element.

## concat Behavior

```cpp
static var concat(Args a)
{
    if (Array<var>* array = a.thisObject.getArray())
    {
        for (int i = 0; i < a.numArguments; i++)
        {
            var newElements = get(a, i);
            for (int j = 0; j < newElements.size(); j++)
                array->insert(-1, newElements[j]);
        }
    }
    return var();
}
```

Note: This modifies the array IN PLACE (unlike JavaScript's `Array.prototype.concat` which returns a new array). The method iterates `newElements.size()` which returns 0 for non-array values, so passing a non-array argument is silently ignored.

## insert Behavior

```cpp
static var insert(Args a)
{
    if (Array<var>* array = a.thisObject.getArray())
    {
        int index = getInt(a, 0);
        for (int i = 1; i < a.numArguments; i++)
            array->insert(index++, get(a, i));
    }
    return var();
}
```

Supports inserting multiple elements at once via variadic arguments.

## slice Behavior

```cpp
static var slice(Args a)
{
    int size = array->size();
    int start = getInt(a, 0);
    int end = a.numArguments > 1 ? getInt(a, 1) : size;

    // Handle negative indices (JS spec)
    if (start < 0) start = jmax(0, size + start);
    if (end < 0) end = jmax(0, size + end);

    start = jmin(start, size);
    end = jmin(end, size);

    var result;
    for (int i = start; i < end; ++i)
        result.append(array->getReference(i));

    return result;
}
```

Supports negative indices per JS spec. Returns a new array (shallow copy).

## isArray -- Static Utility

```cpp
static var isArray(Args a)
{
    return get(a, 0).isArray();
}
```

This is essentially a static method -- it checks its first argument, not `thisObject`. Called as `Array.isArray(someVar)`.

## Doxygen Documentation Class

**File:** `JavascriptEngineObjects.cpp`, lines 675-766

`DoxygenArrayFunctions` is a dummy class that provides Doxygen-parseable documentation for the array API. It is never instantiated -- it exists solely to generate the API reference documentation. The base JSON is generated from this class.

## Key Differences from JavaScript Arrays

1. `concat` modifies in-place (JS returns new array)
2. `sort()` without comparator sorts numerically only (strings compare as equal). JS converts to string and sorts lexicographically.
3. `contains` is the native name; `includes` is the JS-compatible alias
4. `pushIfNotAlreadyThere` and `reserve` are HISE-specific additions
5. `isEmpty` is HISE-specific (not in standard JS)
6. `removeElement` is HISE-specific (JS uses `splice`)
7. `sortNatural` is HISE-specific
8. No `splice`, `reduce`, `flat`, `flatMap`, `fill`, `copyWithin`, `entries`, `values`, `keys` (JS methods not implemented)
9. `indexOf` has a `typeStrictness` parameter not present in JS

## Threading Summary

| Thread Safety | Methods |
|---|---|
| Always warns on audio thread | `join` |
| Warns if would reallocate | `push`, `pushIfNotAlreadyThere` |
| Safe reads | `contains`, `includes`, `indexOf`, `lastIndexOf`, `isEmpty`, `isArray`, `find`, `findIndex`, `some`, `every` |
| Mutating (no guard) | `pop`, `shift`, `remove`, `removeElement`, `clear`, `reverse`, `insert`, `concat`, `sort`, `sortNatural` |
| Pre-allocation | `reserve` (call in onInit) |
