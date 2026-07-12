MarkdownRenderer::getStyleData() -> JSON

Thread safety: UNSAFE -- acquires CriticalSection lock, constructs DynamicObject with heap allocations
Returns the current style configuration as a JSON object. Modify and pass
back to setStyleData() for incremental style changes.

Example return value:
  { "Font": "default", "BoldFont": "default", "FontSize": 18.0,
    "UseSpecialBoldFont": 0,
    "bgColour": 0xFF222222, "textColour": 0xFFCCCCCC,
    "headlineColour": 0xFFFFFFFF, "codeColour": 0xFFBBBBBB,
    "codeBgColour": 0xFF333333, "linkColour": 0xFF8888FF,
    "linkBgColour": 0x00000000, "tableBgColour": 0xFF2A2A2A,
    "tableHeaderBgColour": 0xFF383838, "tableLineColour": 0xFF555555 }

Dispatch/mechanics:
  ScopedLock on action.lock -> renderer.getStyleData()
    -> StyleData::toDynamicObject(colourAsString=false)
    -> constructs DynamicObject with all 14 style properties

Pair with:
  setStyleData -- modify the returned object and pass it back for incremental updates

Source:
  ScriptingGraphics.cpp  MarkdownObject::getStyleData()
    -> ScopedLock(obj->lock)
    -> renderer.getStyleData().toDynamicObject()
  MarkdownLayout.h:66  StyleData struct definition
