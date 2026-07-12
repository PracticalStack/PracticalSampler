Content::createMarkdownRenderer() -> ScriptObject

Thread safety: UNSAFE -- heap-allocates a MarkdownObject.
Creates a MarkdownRenderer for rendering markdown text in ScriptPanel paint routines.
After creation, call setTextBounds() to define the rendering area, then setText() to
set content, and pass to Graphics.drawMarkdownText() inside a paint routine.

Required setup:
  const var md = Content.createMarkdownRenderer();
  md.setTextBounds([0, 0, 400, 300]);
  md.setText("# Hello\nSome **bold** text");

Pair with:
  MarkdownRenderer.setText -- set markdown content
  MarkdownRenderer.setTextBounds -- define rendering area
  Graphics.drawMarkdownText -- draw in paint routine

Source:
  ScriptingApiContent.cpp  Content::createMarkdownRenderer()
    -> new MarkdownObject (heap allocation)
