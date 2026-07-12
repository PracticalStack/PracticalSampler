ScriptLookAndFeel::setStyleSheetProperty(String variableId, NotUndefined value, String type) -> undefined

Thread safety: UNSAFE
Sets a CSS variable on the LAF's additionalProperties ValueTree. The value is converted
to a CSS-compatible string according to the type parameter. Accessible in CSS via
var(--variableId) syntax. Enables dynamic theming between HISEScript and CSS.

Required setup:
  const var laf = Content.createLocalLookAndFeel();
  laf.setInlineStyleSheet("button { background: var(--bgColor); border-radius: var(--radius); }");
  laf.setStyleSheetProperty("bgColor", 0xFF444444, "color");
  laf.setStyleSheetProperty("radius", 5.0, "px");

Dispatch/mechanics:
  ApiHelpers::convertStyleSheetProperty(value, type) converts to CSS string:
    "" = no conversion, "px"/"em"/"vh"/"deg" = append unit suffix,
    "%" = multiply by 100 + "%", "color" = int to "#AARRGGBB",
    "path" = Path to base64 string
  Stores result in additionalProperties ValueTree.

Pair with:
  setInlineStyleSheet/setStyleSheet -- the stylesheet must reference var(--variableId)
    for the property to have any effect

Source:
  ScriptingGraphics.cpp:2655  ScriptedLookAndFeel::setStyleSheetProperty()
    -> ApiHelpers::convertStyleSheetProperty(value, type)
    -> stores in additionalProperties ValueTree
