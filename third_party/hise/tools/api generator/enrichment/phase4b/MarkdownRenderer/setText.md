MarkdownRenderer::setText(String markdownText) -> undefined

Thread safety: UNSAFE -- acquires CriticalSection lock, parses markdown into element objects (heap allocations for each parsed element)
Sets the markdown text to be parsed and rendered. Immediately parses into internal
element objects (headings, paragraphs, code blocks, tables, lists, images).
Replaces previously parsed content entirely.

Dispatch/mechanics:
  ScopedLock on action.lock -> renderer.setNewText(markdownText)
    -> MarkdownParser::parse() -- iterates characters, creates Element objects
    -> supports: headings (1-4), bold, italic, code, fenced code blocks,
       bullet lists, numbered lists, tables, links, images, blockquotes, rules

Pair with:
  setTextBounds -- must call after setText() to get correct height for new content
  setImageProvider -- set before setText() if markdown contains image references
  Graphics.drawMarkdownText -- renders the parsed content in a paint callback

Anti-patterns:
  - Headline levels beyond 4 (##### or more) are silently clamped to level 4.

Source:
  ScriptingGraphics.cpp  MarkdownObject::setText()
    -> ScopedLock(obj->lock)
    -> renderer.setNewText(markdownText)
  Markdown.h:62  MarkdownParser -- core parsing engine
  MarkdownParser.cpp:421  parseBlock() -- character dispatch to element parsers
