# MarkdownRenderer -- C++ Source Exploration

## Resources Consulted

- `enrichment/resources/survey/class_survey.md` -- checked prerequisites (MarkdownRenderer depends on Content, which is NOT enriched yet)
- `enrichment/resources/survey/class_survey_data.json` -- MarkdownRenderer entry: createdBy=[Content], seeAlso=[Graphics, ScriptLabel]
- `enrichment/resources/explorations/Graphics.md` -- brief mention of MarkdownObject at line 637
- No prerequisite Readme available (Content not enriched)

## Class Header Location

**Scripting API wrapper:** `HISE/hi_scripting/scripting/api/ScriptingGraphics.h` lines 555-594  
**C++ class name:** `ScriptingObjects::MarkdownObject` (exposed as `MarkdownRenderer` to script)  
**Implementation:** `HISE/hi_scripting/scripting/api/ScriptingGraphics.cpp` lines 1344-1603  

**Underlying renderer:** `HISE/hi_tools/hi_markdown/MarkdownRenderer.h` lines 40-205  
**Base parser:** `HISE/hi_tools/hi_markdown/Markdown.h` lines 62-410  
**StyleData:** `HISE/hi_tools/hi_markdown/MarkdownLayout.h` lines 66-122  
**Draw action:** `HISE/hi_core/hi_core/MiscComponents.h` lines 370-383  

## Architecture Overview

The scripting API class `MarkdownRenderer` (C++ internal name: `ScriptingObjects::MarkdownObject`) is a lightweight scripting wrapper around a draw action object (`DrawActions::MarkdownAction`). It does NOT directly extend the HISE `MarkdownRenderer` C++ class. Instead, it owns a reference-counted `MarkdownAction::Ptr`, which itself contains:

1. A `hise::MarkdownRenderer renderer` -- the actual C++ renderer
2. A `CriticalSection lock` -- for thread-safe access
3. A `Rectangle<float> area` -- the rendering bounds

The scripting object delegates all operations through this action object.

### Rendering Pipeline

```
Content.createMarkdownRenderer()
    -> new ScriptingObjects::MarkdownObject(pwsc)
        -> new DrawActions::MarkdownAction(stringWidthFunction)
            -> creates MarkdownRenderer("", f) internally

User calls setText(markdownText)
    -> ScopedLock on action.lock
    -> action.renderer.setNewText(markdownText)

User calls setTextBounds([x, y, w, h])
    -> action.area = ApiHelpers::getRectangleFromVar(area)
    -> ScopedLock on action.lock
    -> return action.renderer.getHeightForWidth(area.getWidth())

In paint callback:
    g.drawMarkdownText(markdownRenderer)
    -> GraphicsObject checks area is not empty (script error if so)
    -> drawActionHandler.addDrawAction(obj->obj.get())
    -> During actual rendering: MarkdownAction::perform(g)
        -> ScopedLock on lock
        -> renderer.draw(g, area)
```

The key insight: `MarkdownRenderer` is not a UI component -- it is a **draw action** that gets rendered via `Graphics.drawMarkdownText()` inside a `ScriptPanel.setPaintRoutine()` or LAF paint function. The object accumulates state (text, style, bounds) and then gets rendered as part of the Graphics draw queue.

## Class Declaration

```cpp
// ScriptingGraphics.h:555
namespace ScriptingObjects {
    class MarkdownObject : public ConstScriptingObject
    {
    public:
        MarkdownObject(ProcessorWithScriptingContent* pwsc);
        
        Identifier getObjectName() const override { RETURN_STATIC_IDENTIFIER("MarkdownRenderer"); }
        String getDebugName() const override { return "MarkdownRenderer"; }
        
        Component* createPopupComponent(const MouseEvent& e, Component *c) override;
        
        // API methods
        void setText(const String& markdownText);
        float setTextBounds(var area);
        void setStyleData(var styleData);
        var getStyleData();
        void setImageProvider(var data);
        
        hise::DrawActions::MarkdownAction::Ptr obj;
        
    private:
        class ScriptedImageProvider;
        class Preview;
        struct Wrapper;
    };
}
```

## Constructor

```cpp
ScriptingObjects::MarkdownObject::MarkdownObject(ProcessorWithScriptingContent* pwsc) :
    ConstScriptingObject(pwsc, 0),  // 0 = no constants
    obj(new DrawActions::MarkdownAction(
        std::bind(&MainController::getStringWidthFloat, 
                  pwsc->getMainController_(), 
                  std::placeholders::_1, std::placeholders::_2)))
{
    ADD_API_METHOD_1(setText);
    ADD_API_METHOD_1(setStyleData);
    ADD_API_METHOD_1(setTextBounds);
    ADD_API_METHOD_0(getStyleData);
    ADD_API_METHOD_1(setImageProvider);
}
```

Key observations:
- `ConstScriptingObject(pwsc, 0)` -- **zero constants**. No `addConstant()` calls.
- All methods use `ADD_API_METHOD_N` (plain), NOT `ADD_TYPED_API_METHOD_N`. No forced parameter types.
- The string width function is bound from `MainController::getStringWidthFloat`, which ensures proper font metric calculation across platforms (Windows has known issues with multithreaded typeface pointers -- see the comment in `MarkdownLayout.h` line 60).

## Method Wrapper Registration

```cpp
struct ScriptingObjects::MarkdownObject::Wrapper
{
    API_VOID_METHOD_WRAPPER_1(MarkdownObject, setText);
    API_VOID_METHOD_WRAPPER_1(MarkdownObject, setStyleData);
    API_VOID_METHOD_WRAPPER_1(MarkdownObject, setImageProvider);
    API_METHOD_WRAPPER_1(MarkdownObject, setTextBounds);       // returns float
    API_METHOD_WRAPPER_0(MarkdownObject, getStyleData);         // returns var
};
```

## Factory Method (obtainedVia)

```cpp
// ScriptingApiContent.cpp:9031
juce::var ScriptingApi::Content::createMarkdownRenderer()
{
    return var(new ScriptingObjects::MarkdownObject(getScriptProcessor()));
}
```

Registered in Content constructor:
```cpp
setMethod("createMarkdownRenderer", Wrapper::createMarkdownRenderer);
```

No arguments. Returns a new instance each call. This is a simple factory method on `Content`.

## DrawActions::MarkdownAction

```cpp
// MiscComponents.h:370
struct MarkdownAction: public ActionBase
{
    MarkdownAction(const MarkdownLayout::StringWidthFunction& f);;
    
    SET_ACTION_ID(renderMarkdown);
    
    using Ptr = ReferenceCountedObjectPtr<MarkdownAction>;
    
    void perform(Graphics& g) override;
    
    CriticalSection lock;
    MarkdownRenderer renderer;
    Rectangle<float> area;
};

// MiscComponents.cpp:677
DrawActions::MarkdownAction::MarkdownAction(const MarkdownLayout::StringWidthFunction& f):
    renderer("", f)
{}

void DrawActions::MarkdownAction::perform(Graphics& g)
{
    ScopedLock sl(lock);
    renderer.draw(g, area);
}
```

The `MarkdownAction` inherits from `DrawActions::ActionBase`, which is a `ReferenceCountedObject`. This is the same base used by all Graphics draw operations (fillRect, drawPath, etc.). The key difference from other draw actions is that `MarkdownAction` is **persistent and mutable** -- it is created once, modified via the scripting API methods, and then added to the draw action handler each paint cycle via `g.drawMarkdownText()`.

## Underlying MarkdownRenderer (C++ class)

```cpp
// MarkdownRenderer.h:40
class MarkdownRenderer : public MarkdownParser,
                         public ViewportWithScrollCallback::Listener
{
    // Inheritance: MarkdownParser -> MarkdownRenderer
    // MarkdownParser contains the parsing logic and element storage
    // MarkdownRenderer adds drawing, height calculation, and scroll support
};
```

### Key methods used by the scripting wrapper:

- `setNewText(markdownText)` -- inherited from MarkdownParser, sets the text and triggers parsing
- `getHeightForWidth(float width)` -- calculates total rendered height for given width
- `draw(Graphics& g, Rectangle<float> totalArea)` -- renders all parsed elements
- `setStyleData(MarkdownLayout::StyleData)` -- sets visual styling
- `getStyleData()` -- returns current StyleData reference
- `clearResolvers()` -- removes all image/link resolvers
- `setImageProvider(ImageProvider*)` -- adds a new image provider

## MarkdownParser Base Class

The `MarkdownParser` (`Markdown.h:62`) is the core parsing engine. It:

1. Takes markdown text as a string
2. Parses it into an array of `Element` objects
3. Each element type corresponds to a markdown block element

### Supported Element Types

From `parseBlock()` in `MarkdownParser.cpp:421`:

| Character | Parser Method | Element Type |
|-----------|---------------|--------------|
| `#` | `parseHeadline()` | `Headline` (levels 1-4) |
| `-` | `parseBulletList()` | `BulletPointList` |
| `>` | `parseComment()` | `Comment` (block quote) |
| `` ` `` | `parseJavascriptBlock()` | `CodeBlock` (fenced code) |
| `\|` | `parseTable()` | `MarkdownTable` |
| `!` | `parseImage()` | `ImageElement` |
| `1-9` | `parseEnumeration()` | `EnumerationList` |
| `$` | `parseButton()` | `ActionButton` |
| `_`/`*`/`-` | `parseHorizontalRuler()` | `HorizontalRuler` |
| default | `parseLine()` | `TextBlock` (inline formatting: bold, italic, code, links) |
| `---` (at start) | `parseMarkdownHeader()` | YAML header (not rendered) |

### Inline Formatting (within text blocks)

The parser supports these inline elements (parsed by `parseText()`):
- **Bold**: `**text**`
- *Italic*: `*text*`
- `Code`: `` `text` ``
- Links: `[text](url)` 
- Images: `![alt](url)`

### Headline Levels

Headlines are limited to 4 levels (`headlineLevel = jlimit(1, 4, headlineLevel)`). The font size multipliers are stored in `StyleData::headlineFontSize`:

```cpp
std::array<float, 4> headlineFontSize = { 2.375f, 1.9375f, 1.5f, 1.2f };
```

These multipliers are applied to `fontSize` to compute the headline font size.

## StyleData Schema

The `MarkdownLayout::StyleData` structure is the complete styling configuration. It is serialized to/from a JSON object using `toDynamicObject()`/`fromDynamicObject()`.

### MarkdownStyleIds Namespace

Defined in `MarkdownLayout.h:39`:

```cpp
namespace MarkdownStyleIds
{
    DECLARE_ID(Font);
    DECLARE_ID(BoldFont);
    DECLARE_ID(FontSize);
    DECLARE_ID(bgColour);
    DECLARE_ID(codeBgColour);
    DECLARE_ID(linkBgColour);
    DECLARE_ID(textColour);
    DECLARE_ID(codeColour);
    DECLARE_ID(linkColour);
    DECLARE_ID(headlineColour);
    DECLARE_ID(tableBgColour);
    DECLARE_ID(tableHeaderBgColour);
    DECLARE_ID(tableLineColour);
    DECLARE_ID(UseSpecialBoldFont);
}
```

### StyleData Properties Table

| Property Key | C++ Member | Type | Default (Dark) | Default (Bright) | Description |
|--------------|------------|------|-----------------|-------------------|-------------|
| `Font` | `f` | String | `"default"` (GLOBAL_FONT) | same | Font family name |
| `BoldFont` | `boldFont` | String | `"default"` (GLOBAL_BOLD_FONT) | same | Bold font family name |
| `FontSize` | `fontSize` | float | `18.0` | same | Base font size in pixels |
| `bgColour` | `backgroundColour` | int64 (ARGB) | `0xFF333333` | `0xFFEEEEEE` | Background colour |
| `codeBgColour` | `codebackgroundColour` | int64 (ARGB) | `0x33888888` | same | Code block background |
| `linkBgColour` | `linkBackgroundColour` | int64 (ARGB) | `0x008888FF` | same | Link highlight background |
| `textColour` | `textColour` | int64 (ARGB) | `0xFFFFFFFF` (white) | `0xFF333333` | Normal text colour |
| `codeColour` | `codeColour` | int64 (ARGB) | `0xFFFFFFFF` | `0xFF333333` | Code text colour |
| `linkColour` | `linkColour` | int64 (ARGB) | `0xFFAAAAFF` | `0xFF000044` | Link text colour |
| `headlineColour` | `headlineColour` | int64 (ARGB) | SIGNAL_COLOUR | `0xFF444444` | Headline text colour |
| `tableBgColour` | `tableBgColour` | int64 (ARGB) | grey@0.2 alpha | same | Table cell background |
| `tableHeaderBgColour` | `tableHeaderBackgroundColour` | int64 (ARGB) | grey@0.2 alpha | same | Table header background |
| `tableLineColour` | `tableLineColour` | int64 (ARGB) | grey@0.2 alpha | same | Table border lines |
| `UseSpecialBoldFont` | `useSpecialBoldFont` | bool | `false` | same | Use separate bold typeface |

### fromDynamicObject() Logic

```cpp
bool MarkdownLayout::StyleData::fromDynamicObject(var obj, const std::function<Font(const String&)>& fontLoader)
{
    // Font handling
    auto fName = obj.getProperty(MarkdownStyleIds::Font, "default");
    auto bName = obj.getProperty(MarkdownStyleIds::BoldFont, "default");
    useSpecialBoldFont = obj.getProperty(MarkdownStyleIds::UseSpecialBoldFont, useSpecialBoldFont);
    fontSize = obj.getProperty(MarkdownStyleIds::FontSize, fontSize);
    
    if(fName == "default") f = GLOBAL_FONT();
    else f = fontLoader(fName);
    
    if(bName == "default") { boldFont = GLOBAL_BOLD_FONT(); useSpecialBoldFont = true; }
    else boldFont = fontLoader(bName);
    
    // Colour handling -- uses int64 ARGB values
    auto getColourFromVar = [&](const Identifier& id, Colour defaultColour) {
        if(!obj.hasProperty(id)) return defaultColour;
        auto v = (int64)obj.getProperty(id, (int64)defaultColour.getARGB());
        return Colour((uint32)v);
    };
    
    // All colours loaded via GET_COLOUR macro
    GET_COLOUR(codebackgroundColour, MarkdownStyleIds::codeBgColour);
    // ... etc for all colour properties
    
    return true;
}
```

Key: Font names are resolved through `MainController::getFontFromString()` which looks up loaded fonts. The string `"default"` triggers the global font fallback.

### toDynamicObject() Logic

```cpp
juce::var MarkdownLayout::StyleData::toDynamicObject(bool colourAsString) const
{
    DynamicObject::Ptr obj = new DynamicObject();
    
    // When colourAsString=false (default in scripting API), colours are int64 ARGB
    // When colourAsString=true, colours are hex strings
    auto getColour = [&](const Colour& c) {
        return colourAsString ? var(c.toString()) : var((int64)c.getARGB());
    };
    
    obj->setProperty(MarkdownStyleIds::Font, f.getTypefaceName());
    obj->setProperty(MarkdownStyleIds::BoldFont, boldFont.getTypefaceName());
    obj->setProperty(MarkdownStyleIds::FontSize, fontSize);
    // ... all colour properties
    
    return var(obj.get());
}
```

When called from scripting (`getStyleData()`), `colourAsString` is `false` by default, so colours come back as int64 ARGB hex numbers.

### Non-exposed StyleData fields

These `StyleData` fields exist in C++ but are NOT exposed through the JSON serialization:
- `headlineFontSize` -- fixed array of 4 size multipliers (not in fromDynamicObject/toDynamicObject)
- `margins` -- element type margin overrides (not in fromDynamicObject/toDynamicObject)

## ScriptedImageProvider

The `setImageProvider()` method creates a custom image provider that resolves image markdown links (`![alt](url)`) to actual images. It is defined as an inner class:

```cpp
// ScriptingGraphics.cpp:1394
class ScriptingObjects::MarkdownObject::ScriptedImageProvider 
    : public MarkdownParser::ImageProvider,
      public ControlledObject
```

### Input Data Format

The `data` parameter must be a JSON array of objects. Each object is either a **Path** or **Image** entry:

#### Path Entry

```json
{
    "URL": "![icon](icon_url)",    // The markdown image URL to match
    "Type": "Path",                 // Must be "Path"
    "Data": [pathData],             // Path data (base64 or array)
    "Colour": 0xFF888888            // Colour for the path fill
}
```

Implementation (`ScriptingGraphics.cpp:1428`):
```cpp
struct PathEntry : public Entry
{
    PathEntry(var data): Entry(data)
    {
        jassert(data.getProperty("Type", "").toString() == "Path");
        var pathData = data.getProperty("Data", var());
        ApiHelpers::loadPathFromData(p, pathData);
        c = scriptnode::PropertyHelpers::getColourFromVar(data.getProperty("Colour", 0xFF888888));
    }
    
    Image getImageInternal(float width) override
    {
        // Renders path as a square image of size width x width
        Image img(Image::ARGB, (int)width, (int)width, true);
        Graphics g2(img);
        g2.setColour(c);
        PathFactory::scalePath(p, { 0.0f, 0.0f, width, width });
        g2.fillPath(p);
        return img;
    }
};
```

#### Image Entry

```json
{
    "URL": "![photo](photo_url)",   // The markdown image URL to match
    "Reference": "{PROJECT_FOLDER}image.png"  // Pool reference string
}
```

Implementation (`ScriptingGraphics.cpp:1455`):
```cpp
struct ImageEntry: public ControlledObject, public Entry
{
    ImageEntry(MainController* mc, var data) : ControlledObject(mc), Entry(data)
    {
        auto link = data.getProperty("Reference", "").toString();
        if (link.isNotEmpty())
        {
            PoolReference ref(getMainController(), link, FileHandlerBase::Images);
            pooledImage = getMainController()->getCurrentImagePool()
                ->loadFromReference(ref, PoolHelpers::LoadAndCacheStrong);
        }
    }
    
    Image getImageInternal(float width) override
    {
        if (pooledImage) return *pooledImage.getData();
        return {};
    }
};
```

### URL Matching

The `Entry` base class extracts the URL string and creates a `MarkdownLink`:

```cpp
Entry(var data)
{
    auto urlString = data.getProperty("URL", "").toString();
    if (urlString.isNotEmpty())
        url = MarkdownLink::createWithoutRoot(
            MarkdownLink::Helpers::getSanitizedURL(urlString), 
            MarkdownLink::Image);
}
```

The matching in `getImage()` compares URLs without anchors:
```cpp
if (url.toString(MarkdownLink::UrlWithoutAnchor) == 
    urlToResolve.toString(MarkdownLink::UrlWithoutAnchor))
```

### Priority

```cpp
MarkdownParser::ResolveType getPriority() const override { return MarkdownParser::ResolveType::EmbeddedPath; };
```

`EmbeddedPath` is the highest priority level (value 5, "Always generated"), so scripted image providers take precedence over all other resolvers.

### setImageProvider replaces all resolvers

```cpp
void ScriptingObjects::MarkdownObject::setImageProvider(var data)
{
    auto newProvider = new ScriptedImageProvider(
        getScriptProcessor()->getMainController_(), &obj->renderer, data);
    
    ScopedLock sl(obj->lock);
    obj->renderer.clearResolvers();    // Clears ALL existing resolvers
    obj->renderer.setImageProvider(newProvider);
}
```

Note: `clearResolvers()` removes both image providers AND link resolvers. This means after calling `setImageProvider()`, only the scripted provider remains -- no fallback providers.

## Threading and Locking

All methods that touch the internal renderer use `ScopedLock sl(obj->lock)`:

- `setText()` -- acquires lock, then calls `setNewText()`
- `setTextBounds()` -- sets area first (no lock needed for area), then acquires lock for `getHeightForWidth()`
- `setStyleData()` -- constructs StyleData first, then acquires lock to apply it
- `getStyleData()` -- acquires lock, returns dynamic object
- `setImageProvider()` -- creates provider first, then acquires lock to clear and set
- `MarkdownAction::perform()` (rendering) -- acquires lock for drawing

This means the MarkdownRenderer is safe to use from the scripting thread (onInit, timer, etc.) while it is being rendered on the paint thread. The CriticalSection ensures mutual exclusion.

## Interaction with Graphics.drawMarkdownText()

```cpp
// ScriptingGraphics.cpp:2165
void ScriptingObjects::GraphicsObject::drawMarkdownText(var markdownRenderer)
{
    if (auto obj = dynamic_cast<MarkdownObject*>(markdownRenderer.getObject()))
    {
        if (obj->obj->area.isEmpty())
            reportScriptError("You have to call setTextBounds() before using this method");
        
        drawActionHandler.addDrawAction(obj->obj.get());
    }
    else
        reportScriptError("not a markdown renderer");
}
```

Critical constraint: `setTextBounds()` MUST be called before `drawMarkdownText()` or a script error is thrown. The area check is `obj->obj->area.isEmpty()`.

## Area/Bounds Format

The `setTextBounds()` method accepts the standard HISE rectangle format via `ApiHelpers::getRectangleFromVar()`:

1. **Array**: `[x, y, width, height]` -- 4-element array of numbers
2. **Rectangle object**: A `ScriptingObjects::ScriptRectangle` (created by `Content.createRectangle()`)

Returns: `float` -- the calculated height needed to render the markdown at the given width. This may be more or less than the height specified in the area.

## Preview Component (Backend only)

```cpp
Component* ScriptingObjects::MarkdownObject::createPopupComponent(const MouseEvent& e, Component *c)
{
#if USE_BACKEND
    return new Preview(this);
#else
    ignoreUnused(e, c);
    return nullptr;
#endif
}
```

The Preview class (`ScriptingGraphics.cpp:1344`) is a `ComponentForDebugInformation` with a `PooledUIUpdater::SimpleTimer` that periodically repaints itself with the current markdown content. This only appears in the HISE IDE when right-clicking the variable in the debug console.

## Preprocessor Guards

- `USE_BACKEND` -- only for the `createPopupComponent()` debug preview. All API methods work in both backend and frontend builds.
- No `HISE_INCLUDE_*` guards affect MarkdownRenderer.

## MarkdownParser Element Hierarchy

The parsed markdown elements inherit from `MarkdownParser::Element`:

```cpp
struct Element
{
    virtual void draw(Graphics& g, Rectangle<float> area) = 0;
    virtual float getHeightForWidth(float width) = 0;
    virtual float getTopMargin() const = 0;
    virtual Component* createComponent(int maxWidth);
    
    Array<HyperLink> hyperLinks;
    // ...
};
```

Element types (from `Markdown.h:350`):
- `TextBlock` -- paragraph text with inline formatting
- `Headline` -- h1-h4 headings
- `BulletPointList` -- unordered lists
- `Comment` -- blockquotes
- `CodeBlock` -- fenced code blocks
- `MarkdownTable` -- pipe-separated tables
- `ImageElement` -- embedded images
- `EnumerationList` -- ordered lists (1. 2. 3.)
- `ActionButton` -- `$[text](link)` interactive buttons
- `HorizontalRuler` -- horizontal rules (`---`, `***`, `___`)
- `LiveCodeBlock` -- live-updating code blocks (internal)
- `ContentFooter` -- auto-generated footer

## Rendering Flow (MarkdownRenderer::draw)

```cpp
void MarkdownRenderer::draw(Graphics& g, Rectangle<float> totalArea, Rectangle<int> viewedArea) const
{
    for (auto* e : elements)
    {
        auto heightToUse = e->getHeightForWidthCached(totalArea.getWidth());
        auto topMargin = e->getTopMargin();
        totalArea.removeFromTop((float)topMargin);
        auto ar = totalArea.removeFromTop(heightToUse);
        
        if (firstDraw || viewedArea.isEmpty() || ar.toNearestInt().intersects(viewedArea))
            e->draw(g, ar);
    }
    firstDraw = false;
}
```

Elements are laid out top-to-bottom with their natural heights and top margins. The `viewedArea` parameter enables culling for viewport-based scrolling, but when called from the scripting API through `MarkdownAction::perform()`, `viewedArea` is empty (default `{}`), so ALL elements are drawn every frame.

## getHeightForWidth Flow

```cpp
float MarkdownRenderer::getHeightForWidth(float width, bool forceUpdate)
{
    if(width == 0.0f && lastHeight > 0.0f) return lastHeight;
    if (width == lastWidth && !forceUpdate) return lastHeight;  // Cached!
    
    float height = 0.0f;
    for (auto* e : elements)
    {
        // Headline anchor tracking for scrolling
        if (auto h = dynamic_cast<MarkdownParser::Headline*>(e))
            h->anchorY = height;
        
        height += e->getTopMargin();
        height += e->getHeightForWidthCached(width, forceUpdate);
    }
    
    lastWidth = width;
    lastHeight = height;
    firstDraw = true;
    return height;
}
```

The height calculation is cached -- calling `setTextBounds()` with the same width repeatedly will return the cached value. This is efficient for repaint scenarios.

## Relationship to Content and Graphics

MarkdownRenderer sits at the intersection of Content (factory) and Graphics (consumer):

1. **Created by Content:** `Content.createMarkdownRenderer()` -- typically called in `onInit`
2. **Configured via API:** `setText()`, `setStyleData()`, `setTextBounds()`, `setImageProvider()` -- can be called anytime
3. **Rendered by Graphics:** `g.drawMarkdownText(markdownRenderer)` -- called inside paint callbacks

This is similar to the `Path` object pattern: created once, configured, then passed to Graphics for rendering. Unlike Path, the MarkdownRenderer has a persistent area and produces different visual output depending on its text content.

## MarkdownRenderer vs ScriptLabel

Key distinction: MarkdownRenderer requires manual rendering via Graphics in a paint callback. ScriptLabel is a UI component placed directly on the interface. MarkdownRenderer supports full markdown syntax (headings, lists, code blocks, tables, images). ScriptLabel is plain text only with basic alignment.
