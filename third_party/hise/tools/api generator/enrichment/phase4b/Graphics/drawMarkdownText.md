Graphics::drawMarkdownText(ScriptObject markdownRenderer) -> undefined

Thread safety: UNSAFE -- accesses the MarkdownRenderer's internal draw action
Draws the text of a MarkdownRenderer object. The renderer must have setTextBounds()
called first. The Graphics object's current colour and font do NOT affect markdown
rendering -- the MarkdownRenderer uses its own styling.

Required setup:
  const var md = Content.createMarkdownRenderer();
  md.setText("**Hello** World");
  md.setTextBounds([10, 10, 200, 100]);

Anti-patterns:
  - Must call setTextBounds() on the MarkdownRenderer before this method --
    otherwise triggers "You have to call setTextBounds() before using this method"
  - Not a valid MarkdownRenderer triggers "not a markdown renderer"

Source:
  ScriptingGraphics.cpp  GraphicsObject::drawMarkdownText()
    -> retrieves MarkdownRenderer's internal draw action
    -> adds it to the handler's action list
