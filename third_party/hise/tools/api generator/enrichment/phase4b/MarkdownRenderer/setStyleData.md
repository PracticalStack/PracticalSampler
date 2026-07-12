MarkdownRenderer::setStyleData(JSON styleData) -> undefined

Thread safety: UNSAFE -- constructs StyleData from JSON (font lookups, string operations), then acquires CriticalSection lock to apply
Sets the visual style configuration. See getStyleData() for the full object
format and default values. Properties not included are reset to defaults --
to modify individual properties without resetting others, use getStyleData()
first.

Dispatch/mechanics:
  Constructs fresh StyleData with defaults
  -> StyleData::fromDynamicObject(obj, fontLoader) populates from JSON
  -> font names resolved via MainController::getFontFromString()
  -> ScopedLock on action.lock -> renderer.setStyleData(newStyle)

Pair with:
  getStyleData -- get current style, modify, then pass back for incremental changes

Anti-patterns:
  - Passing a partial object with only the properties you want to change -- all
    unspecified properties reset to defaults. Always getStyleData() first, modify,
    then setStyleData().
  - Setting BoldFont to "default" forces useSpecialBoldFont=true regardless of the
    UseSpecialBoldFont property in the JSON. Passing UseSpecialBoldFont:false
    alongside BoldFont:"default" is silently overridden.

Source:
  ScriptingGraphics.cpp  MarkdownObject::setStyleData()
    -> StyleData::fromDynamicObject(obj, fontLoader)
    -> ScopedLock(obj->lock) -> renderer.setStyleData(sd)
  MarkdownLayout.h:66  StyleData::fromDynamicObject() -- font resolution + colour loading
