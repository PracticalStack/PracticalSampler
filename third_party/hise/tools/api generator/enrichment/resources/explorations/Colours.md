# Colours -- C++ Source Exploration

## Resources Consulted

- `enrichment/resources/survey/class_survey.md` -- No prerequisites for Colours
- `enrichment/resources/survey/class_survey_data.json` -- Colours entry (no `createdBy`, no `creates`, `seeAlso`: Graphics)
- `enrichment/base/Colours.json` -- 12 methods

No prerequisite classes. Colours has no enrichment prerequisites in the survey table.

---

## Class Declaration

**File:** `HISE/hi_scripting/scripting/api/ScriptingApi.h`, line 1923

```cpp
class Colours: public ApiClass
{
public:
    Colours();
    ~Colours() {};
    Identifier getObjectName() const override { RETURN_STATIC_IDENTIFIER("Colours"); }

    // 12 API methods (7 withX-style modifiers, 2 vec4 converters, 2 HSL converters, 1 mix)
    int withAlpha(var colour, float alpha);
    int withHue(var colour, float hue);
    int withSaturation(var colour, float saturation);
    int withBrightness(var colour, float brightness);
    int withMultipliedAlpha(var colour, float factor);
    int withMultipliedSaturation(var colour, float factor);
    int withMultipliedBrightness(var colour, float factor);
    var toVec4(var colour);
    int fromVec4(var vec4);
    int mix(var colour1, var colour2, float alpha);
    var toHsl(var colour);
    int fromHsl(var hsl);

    struct Wrapper;
private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Colours);
};
```

### Inheritance

- Direct base: `ApiClass` (defined in `hi_scripting/scripting/engine/JavascriptApiClass.h` line 350)
- `ApiClass` extends `ReferenceCountedObject` and provides the method registration infrastructure (`addFunction1..5`, `addConstant`, `setFunctionIsInlineable`)
- No `ConstScriptingObject` -- this class has NO reference to a `ProcessorWithScriptingContent` or `MainController`. It is purely stateless.

### Key Properties

- **No member variables** -- no state whatsoever. All methods are pure functions that take input and return output.
- **No threading concerns** -- stateless, can be called from any thread.
- **No lifecycle constraints** -- works anywhere, anytime.
- **No preprocessor guards** -- no `USE_BACKEND` or other conditional compilation.

---

## Registration

**File:** `HISE/hi_scripting/scripting/ScriptProcessorModules.cpp`, line 316

```cpp
scriptEngine->registerApiClass(new ScriptingApi::Colours());
```

Registered as a global namespace API class (like `Math`, `Console`, `FileSystem`, etc.), accessible as `Colours` from any script. No factory method needed -- it's a built-in namespace.

---

## Constructor -- Constants (139 Named Colours)

**File:** `HISE/hi_scripting/scripting/api/ScriptingApi.cpp`, line 7094

```cpp
ScriptingApi::Colours::Colours() :
ApiClass(139)
```

The constructor argument `139` is the pre-allocated size for the combined constants + methods table. The constructor adds 127 named colour constants via `addConstant()`, all as `int64` ARGB values.

### Complete Colour Constant Table

All values are ARGB uint32 stored as `int64`. The alpha channel is fully opaque (0xFF) for all except `transparentBlack` (0x00000000) and `transparentWhite` (0x00FFFFFF).

| Name | Hex Value | Notes |
|------|-----------|-------|
| transparentBlack | 0x00000000 | Alpha = 0 |
| transparentWhite | 0x00ffffff | Alpha = 0 |
| aliceblue | 0xfff0f8ff | |
| antiquewhite | 0xfffaebd7 | |
| aqua | 0xff00ffff | Same as cyan |
| aquamarine | 0xff7fffd4 | |
| azure | 0xfff0ffff | |
| beige | 0xfff5f5dc | |
| bisque | 0xffffe4c4 | |
| black | 0xff000000 | |
| blanchedalmond | 0xffffebcd | |
| blue | 0xff0000ff | |
| blueviolet | 0xff8a2be2 | |
| brown | 0xffa52a2a | |
| burlywood | 0xffdeb887 | |
| cadetblue | 0xff5f9ea0 | |
| chartreuse | 0xff7fff00 | |
| chocolate | 0xffd2691e | |
| coral | 0xffff7f50 | |
| cornflowerblue | 0xff6495ed | |
| cornsilk | 0xfffff8dc | |
| crimson | 0xffdc143c | |
| cyan | 0xff00ffff | Same as aqua |
| darkblue | 0xff00008b | |
| darkcyan | 0xff008b8b | |
| darkgoldenrod | 0xffb8860b | |
| darkgrey | 0xff555555 | Non-standard (CSS darkgray = 0xA9A9A9) |
| darkgreen | 0xff006400 | |
| darkkhaki | 0xffbdb76b | |
| darkmagenta | 0xff8b008b | |
| darkolivegreen | 0xff556b2f | |
| darkorange | 0xffff8c00 | |
| darkorchid | 0xff9932cc | |
| darkred | 0xff8b0000 | |
| darksalmon | 0xffe9967a | |
| darkseagreen | 0xff8fbc8f | |
| darkslateblue | 0xff483d8b | |
| darkslategrey | 0xff2f4f4f | |
| darkturquoise | 0xff00ced1 | |
| darkviolet | 0xff9400d3 | |
| deeppink | 0xffff1493 | |
| deepskyblue | 0xff00bfff | |
| dimgrey | 0xff696969 | |
| dodgerblue | 0xff1e90ff | |
| firebrick | 0xffb22222 | |
| floralwhite | 0xfffffaf0 | |
| forestgreen | 0xff228b22 | |
| fuchsia | 0xffff00ff | Same as magenta |
| gainsboro | 0xffdcdcdc | |
| gold | 0xffffd700 | |
| goldenrod | 0xffdaa520 | |
| grey | 0xff808080 | |
| green | 0xff008000 | |
| greenyellow | 0xffadff2f | |
| honeydew | 0xfff0fff0 | |
| hotpink | 0xffff69b4 | |
| indianred | 0xffcd5c5c | |
| indigo | 0xff4b0082 | |
| ivory | 0xfffffff0 | |
| khaki | 0xfff0e68c | |
| lavender | 0xffe6e6fa | |
| lavenderblush | 0xfffff0f5 | |
| lemonchiffon | 0xfffffacd | |
| lightblue | 0xffadd8e6 | |
| lightcoral | 0xfff08080 | |
| lightcyan | 0xffe0ffff | |
| lightgoldenrodyellow | 0xfffafad2 | |
| lightgreen | 0xff90ee90 | |
| lightgrey | 0xffd3d3d3 | |
| lightpink | 0xffffb6c1 | |
| lightsalmon | 0xffffa07a | |
| lightseagreen | 0xff20b2aa | |
| lightskyblue | 0xff87cefa | |
| lightslategrey | 0xff778899 | |
| lightsteelblue | 0xffb0c4de | |
| lightyellow | 0xffffffe0 | |
| lime | 0xff00ff00 | |
| limegreen | 0xff32cd32 | |
| linen | 0xfffaf0e6 | |
| magenta | 0xffff00ff | Same as fuchsia |
| maroon | 0xff800000 | |
| mediumaquamarine | 0xff66cdaa | |
| mediumblue | 0xff0000cd | |
| mediumorchid | 0xffba55d3 | |
| mediumpurple | 0xff9370db | |
| mediumseagreen | 0xff3cb371 | |
| mediumslateblue | 0xff7b68ee | |
| mediumspringgreen | 0xff00fa9a | |
| mediumturquoise | 0xff48d1cc | |
| mediumvioletred | 0xffc71585 | |
| midnightblue | 0xff191970 | |
| mintcream | 0xfff5fffa | |
| mistyrose | 0xffffe4e1 | |
| navajowhite | 0xffffdead | |
| navy | 0xff000080 | |
| oldlace | 0xfffdf5e6 | |
| olive | 0xff808000 | |
| olivedrab | 0xff6b8e23 | |
| orange | 0xffffa500 | |
| orangered | 0xffff4500 | |
| orchid | 0xffda70d6 | |
| palegoldenrod | 0xffeee8aa | |
| palegreen | 0xff98fb98 | |
| paleturquoise | 0xffafeeee | |
| palevioletred | 0xffdb7093 | |
| papayawhip | 0xffffefd5 | |
| peachpuff | 0xffffdab9 | |
| peru | 0xffcd853f | |
| pink | 0xffffc0cb | |
| plum | 0xffdda0dd | |
| powderblue | 0xffb0e0e6 | |
| purple | 0xff800080 | |
| red | 0xffff0000 | |
| rosybrown | 0xffbc8f8f | |
| royalblue | 0xff4169e1 | |
| saddlebrown | 0xff8b4513 | |
| salmon | 0xfffa8072 | |
| sandybrown | 0xfff4a460 | |
| seagreen | 0xff2e8b57 | |
| seashell | 0xfffff5ee | |
| sienna | 0xffa0522d | |
| silver | 0xffc0c0c0 | |
| skyblue | 0xff87ceeb | |
| slateblue | 0xff6a5acd | |
| slategrey | 0xff708090 | |
| snow | 0xfffffafa | |
| springgreen | 0xff00ff7f | |
| steelblue | 0xff4682b4 | |
| tan | 0xffd2b48c | |
| teal | 0xff008080 | |
| thistle | 0xffd8bfd8 | |
| tomato | 0xffff6347 | |
| turquoise | 0xff40e0d0 | |
| violet | 0xffee82ee | |
| wheat | 0xfff5deb3 | |
| white | 0xffffffff | |
| whitesmoke | 0xfff5f5f5 | |
| yellow | 0xffffff00 | |
| yellowgreen | 0xff9acd32 | |

Total: 127 named colour constants. This follows the CSS/X11 named colour set (with British spelling: "grey" not "gray").

**Notable: `darkgrey` uses 0xff555555 which differs from the standard CSS `darkgray` value of 0xffa9a9a9.** This appears to be a HISE-specific override to provide a darker grey that is more useful for audio UI.

---

## Method Registration -- All Inlineable

All 12 methods are registered using `ADD_INLINEABLE_API_METHOD_N` macros (lines 7237-7248):

```cpp
ADD_INLINEABLE_API_METHOD_2(withAlpha);
ADD_INLINEABLE_API_METHOD_2(withHue);
ADD_INLINEABLE_API_METHOD_2(withBrightness);
ADD_INLINEABLE_API_METHOD_2(withSaturation);
ADD_INLINEABLE_API_METHOD_2(withMultipliedAlpha);
ADD_INLINEABLE_API_METHOD_2(withMultipliedBrightness);
ADD_INLINEABLE_API_METHOD_2(withMultipliedSaturation);
ADD_INLINEABLE_API_METHOD_3(mix);
ADD_INLINEABLE_API_METHOD_1(toVec4);
ADD_INLINEABLE_API_METHOD_1(fromVec4);
ADD_INLINEABLE_API_METHOD_1(toHsl);
ADD_INLINEABLE_API_METHOD_1(fromHsl);
```

The `ADD_INLINEABLE_API_METHOD_N` macro does:
1. Registers the function via `addFunctionN(Identifier(#name), &Wrapper::name)`
2. Calls `setFunctionIsInlineable(Identifier(#name))` -- marks the function for JIT inlining optimization

**No typed method registrations (`ADD_TYPED_API_METHOD_N`) are used.** All parameters use the generic `var` wrapper type.

---

## Wrapper Struct

**File:** `HISE/hi_scripting/scripting/api/ScriptingApi.cpp`, lines 7078-7092

All wrappers use `API_METHOD_WRAPPER_N` (non-void). All methods return `int` (ARGB colour) or `var` (float arrays), so they all use the non-void wrapper.

```cpp
struct ScriptingApi::Colours::Wrapper
{
    API_METHOD_WRAPPER_2(Colours, withAlpha);
    API_METHOD_WRAPPER_2(Colours, withHue);
    API_METHOD_WRAPPER_2(Colours, withBrightness);
    API_METHOD_WRAPPER_2(Colours, withSaturation);
    API_METHOD_WRAPPER_2(Colours, withMultipliedAlpha);
    API_METHOD_WRAPPER_2(Colours, withMultipliedBrightness);
    API_METHOD_WRAPPER_2(Colours, withMultipliedSaturation);
    API_METHOD_WRAPPER_1(Colours, fromVec4);
    API_METHOD_WRAPPER_1(Colours, toVec4);
    API_METHOD_WRAPPER_3(Colours, mix);
    API_METHOD_WRAPPER_1(Colours, toHsl);
    API_METHOD_WRAPPER_1(Colours, fromHsl);
};
```

---

## Critical Helper: getCleanedObjectColour / getColourFromVar

Almost every method delegates colour input parsing to `Content::Helpers::getCleanedObjectColour`, which in turn calls `ApiHelpers::getColourFromVar`.

### ApiHelpers::getColourFromVar

**File:** `HISE/hi_scripting/scripting/api/ScriptingApi.cpp`, line 226

```cpp
Colour ApiHelpers::getColourFromVar(const var& value)
{
    int64 colourValue = 0;

    if (value.isInt64() || value.isInt())
        colourValue = (int64)value;
    else if (value.isString())
    {
        auto string = value.toString();

        if (string.startsWith("0x"))
            colourValue = string.getHexValue64();
        else
            colourValue = string.getLargeIntValue();
    }

    return Colour((uint32)colourValue);
}
```

This means colour input parameters (`var colour`) accept:
1. **Integer/int64** -- direct ARGB uint32 value (e.g., `0xFFFF0000` for red)
2. **Hex string** -- `"0xFFFF0000"` parsed as hex
3. **Decimal string** -- parsed as large integer

**Note:** If the input is a float, undefined, or any other type, `colourValue` stays 0, yielding transparent black (0x00000000). There is no error reporting for invalid input.

---

## Method Implementation Patterns

### Group 1: Absolute HSB Modifiers (withAlpha, withHue, withSaturation, withBrightness)

All follow the same pattern:
```cpp
int ScriptingApi::Colours::withAlpha(var colour, float alpha)
{
    auto c = Content::Helpers::getCleanedObjectColour(colour);
    return (int)c.withAlpha(jlimit(0.0f, 1.0f, alpha)).getARGB();
}
```

- Input `colour` parsed via `getCleanedObjectColour`
- Float parameter clamped to [0.0, 1.0] via `jlimit`
- Delegates to JUCE `Colour::withAlpha/withHue/withSaturation/withBrightness`
- Returns `(int)` cast of `getARGB()` -- ARGB uint32 as signed int

### Group 2: Multiplied Modifiers (withMultipliedAlpha, withMultipliedSaturation, withMultipliedBrightness)

```cpp
int ScriptingApi::Colours::withMultipliedAlpha(var colour, float factor)
{
    auto c = Content::Helpers::getCleanedObjectColour(colour);
    return (int)c.withMultipliedAlpha(jmax(0.0f, factor)).getARGB();
}
```

- Factor clamped to >= 0 via `jmax(0.0f, factor)` (no upper bound -- can go above 1.0)
- Delegates to JUCE `Colour::withMultipliedAlpha/withMultipliedSaturation/withMultipliedBrightness`

### Group 3: Vec4 Conversion (toVec4, fromVec4)

**toVec4** returns `[r, g, b, a]` as floats 0.0-1.0:
```cpp
var ScriptingApi::Colours::toVec4(var colour)
{
    auto c = Content::Helpers::getCleanedObjectColour(colour);
    Array<var> v4;
    v4.add(c.getFloatRed());
    v4.add(c.getFloatGreen());
    v4.add(c.getFloatBlue());
    v4.add(c.getFloatAlpha());
    return v4;
}
```

**fromVec4** expects `[r, g, b, a]` as floats 0.0-1.0, converts to 0-255 bytes:
```cpp
int ScriptingApi::Colours::fromVec4(var vec4)
{
    if (vec4.isArray() && vec4.size() == 4)
    {
        auto r = (uint8)roundToInt((float)vec4[0] * 255.0f);
        auto g = (uint8)roundToInt((float)vec4[1] * 255.0f);
        auto b = (uint8)roundToInt((float)vec4[2] * 255.0f);
        auto a = (uint8)roundToInt((float)vec4[3] * 255.0f);
        return Colour(r, g, b, a).getARGB();
    }
    return 0;  // Returns transparent black for invalid input
}
```

- Validates: must be array of exactly 4 elements
- Returns 0 (transparent black) if validation fails -- no error thrown

### Group 4: HSL Conversion (toHsl, fromHsl)

**toHsl** returns `[h, s, l, a]` as floats 0.0-1.0:
```cpp
var ScriptingApi::Colours::toHsl(var colour)
{
    auto c = Content::Helpers::getCleanedObjectColour(colour);
    float hue, saturation, lightness;
    c.getHSL(hue, saturation, lightness);
    Array<var> hsl;
    hsl.add(hue);
    hsl.add(saturation);
    hsl.add(lightness);
    hsl.add(c.getFloatAlpha());
    return hsl;
}
```

**fromHsl** -- CRITICAL ASYMMETRY:
```cpp
int ScriptingApi::Colours::fromHsl(var hsl)
{
    if (hsl.isArray() && hsl.size() == 4)
        return Colour().fromHSL((float)hsl[0], (float)hsl[1], (float)hsl[2], (uint8)(int)hsl[3]).getARGB();
    return 0;
}
```

**The alpha parameter is cast as `(uint8)(int)hsl[3]`**, which treats it as a 0-255 integer value. But `toHsl` outputs alpha as a 0.0-1.0 float. This means:
- `toHsl(0xFFFF0000)` returns `[0.0, 1.0, 0.5, 1.0]` (alpha = 1.0)
- `fromHsl([0.0, 1.0, 0.5, 1.0])` casts alpha as `(uint8)(int)1.0f` = 1, yielding alpha = 1/255 (nearly transparent)

**Roundtrip `fromHsl(toHsl(colour))` is NOT lossless for alpha.** To get full opacity, pass alpha as 255 integer, not 1.0 float.

However, note that JUCE's `Colour::fromHSL` actually takes `float alpha` (0.0-1.0), not `uint8`. So the `(uint8)(int)` cast converts the value to a byte, then JUCE's `fromHSL` receives it as a float parameter via implicit conversion. The cast `(uint8)(int)1.0` = 1, then passed as float alpha = 1.0/255.0? Actually, looking more carefully at the JUCE API: `Colour::fromHSL(float h, float s, float l, float alpha)`. So `(uint8)(int)hsl[3]` where `hsl[3] = 1.0` gives `(uint8)1` = 1, then implicit conversion to float = 1.0f. Wait -- `uint8` value 1 promoted to float is 1.0f. So actually `fromHSL(h, s, l, 1.0f)` which would be full alpha. Let me re-check...

Actually, the `(uint8)(int)` cast means: `(int)1.0` = 1, then `(uint8)1` = 1. Then JUCE receives this as float parameter. C++ implicit conversion: `uint8(1)` -> `float(1.0f)`. So for alpha = 1.0, the roundtrip works: 1.0 -> (int)1 -> (uint8)1 -> float 1.0f.

But for alpha = 0.5 (half transparent): 0.5 -> (int)0 -> (uint8)0 -> float 0.0f. So the alpha is lost.

The asymmetry is real: fractional alpha values between 0 and 1 will be truncated to 0 by the int cast.

### Group 5: Mix (mix)

```cpp
int ScriptingApi::Colours::mix(var colour1, var colour2, float alpha)
{
    auto c1 = Content::Helpers::getCleanedObjectColour(colour1);
    auto c2 = Content::Helpers::getCleanedObjectColour(colour2);
    return c1.interpolatedWith(c2, alpha).getARGB();
}
```

- `alpha` parameter is NOT clamped (unlike the `with*` methods)
- Delegates to JUCE `Colour::interpolatedWith` which handles the interpolation in ARGB space
- `alpha = 0.0` returns colour1, `alpha = 1.0` returns colour2

---

## Colour Representation in HISE

Colours in HiseScript are represented as 32-bit ARGB integers. The format is `0xAARRGGBB`:
- Bits 24-31: Alpha (0x00 = transparent, 0xFF = opaque)
- Bits 16-23: Red
- Bits 8-15: Green
- Bits 0-7: Blue

These integers are stored as `int` in the scripting engine (signed 32-bit), but the underlying JUCE `Colour` class uses `uint32`. The `(int)` cast in return statements handles this conversion.

### Input Format Flexibility

Via `ApiHelpers::getColourFromVar`, colour parameters accept:
- Named constants: `Colours.red` (resolves to `int64` via `addConstant`)
- Hex integers: `0xFFFF0000`
- Hex strings: `"0xFFFF0000"`
- Integer colour values from other Colours methods

---

## Usage Context

The `Colours` namespace is used by:
- **Graphics** class -- `setColour()`, `setGradientFill()`, `drawDropShadow()` etc. all accept colour values
- **ScriptComponent** property system -- colour properties (bgColour, textColour, etc.)
- **ScriptLookAndFeel** paint functions -- LAF callbacks receive and manipulate colours
- **MarkdownRenderer** -- text colour, link colour, code colour, headline colour
- **CSS system** -- colour parsing in the stylesheet engine

The `getCleanedObjectColour` / `getColourFromVar` helper is used 33+ times across the codebase, making it the universal colour input parser.

---

## No Preprocessor Guards

The entire Colours class is unconditionally compiled. No `#if USE_BACKEND`, no `HISE_INCLUDE_*` guards. Available in all build targets.

---

## Summary of Findings

- **12 methods**, all stateless pure functions
- **127 named colour constants** (CSS/X11 colour set, British spelling)
- **All methods inlineable** -- `ADD_INLINEABLE_API_METHOD_N` used for all
- **No typed parameter registrations** -- all use generic `var` wrapper
- **Key helper:** `ApiHelpers::getColourFromVar` for flexible colour input parsing
- **fromHsl alpha asymmetry:** `toHsl` outputs alpha as 0.0-1.0 float, but `fromHsl` casts alpha as `(uint8)(int)` which truncates fractional values to 0
- **No error reporting** -- invalid inputs silently return 0 (transparent black)
- **Clamping behavior differs:** absolute modifiers use `jlimit(0,1,x)`, multiplied modifiers use `jmax(0,x)` (no upper bound), `mix` has no clamping at all
