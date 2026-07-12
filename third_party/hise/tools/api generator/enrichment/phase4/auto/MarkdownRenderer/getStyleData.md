Returns the current style configuration as a JSON object. Use this to read the defaults before modifying individual properties - passing the modified object back to `setStyleData()` preserves any properties you did not change.

```json
{
  "Font": "default",
  "BoldFont": "default",
  "FontSize": 18.0,
  "UseSpecialBoldFont": 0,
  "bgColour": 0xFF222222,
  "textColour": 0xFFCCCCCC,
  "headlineColour": 0xFFFFFFFF,
  "codeColour": 0xFFBBBBBB,
  "codeBgColour": 0xFF333333,
  "linkColour": 0xFF8888FF,
  "linkBgColour": 0x00000000,
  "tableBgColour": 0xFF2A2A2A,
  "tableHeaderBgColour": 0xFF383838,
  "tableLineColour": 0xFF555555
}
```
