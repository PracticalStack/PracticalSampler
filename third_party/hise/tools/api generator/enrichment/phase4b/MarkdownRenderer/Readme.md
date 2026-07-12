MarkdownRenderer (object)
Obtain via: Content.createMarkdownRenderer()

Stateful markdown parser and draw action. Parses markdown text into styled
visual elements (headings, lists, tables, code blocks, images) and renders
them via Graphics.drawMarkdownText() inside a ScriptPanel paint routine or
LAF function. Not a UI component -- it is a draw action object.

Complexity tiers:
  1. Basic styled text: setText, setTextBounds + g.drawMarkdownText(). Default
     style. Simple rich text with bold/italic/headings.
  2. Themed rendering: + getStyleData, setStyleData. Custom fonts, colours,
     table appearance to match a UI theme.
  3. Rich content with images: + setImageProvider. Embed vector icons or pool
     images inline via markdown image syntax. Dynamic height calculation for
     auto-sizing containers.

Practical defaults:
  - Use getStyleData(), modify the returned object, then setStyleData() to
    preserve defaults for properties you do not change (setStyleData resets
    unspecified properties).
  - FontSize 13-14 for tooltip/secondary text; default 18.0 for dialog body.
  - Set tableLineColour to 0 (transparent) when table borders are unwanted.
  - Store as const var at file/namespace scope. Creating once and calling
    setText() to update is much cheaper than creating per paint cycle.

Common mistakes:
  - Calling g.drawMarkdownText(md) without md.setTextBounds() first -- throws
    script error "You have to call setTextBounds() before using this method".
  - Passing a partial object to setStyleData() -- resets all unspecified
    properties to defaults. Use getStyleData() first, modify, then set back.
  - Calling setText() after setTextBounds() and using the old height -- the
    returned height is stale. Call setTextBounds() again after setText().
  - Creating a new MarkdownRenderer inside setPaintRoutine() -- allocates on
    every repaint. Create once in onInit, update via setText().

Example:
  const var md = Content.createMarkdownRenderer();
  md.setText("## Hello\nThis is **bold** and *italic* text.");
  md.setTextBounds([0, 0, 400, 300]);

Methods (5):
  getStyleData      setImageProvider    setStyleData
  setText           setTextBounds
