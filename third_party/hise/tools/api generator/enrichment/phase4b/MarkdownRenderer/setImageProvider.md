MarkdownRenderer::setImageProvider(Array data) -> undefined

Thread safety: UNSAFE -- creates image provider objects (heap allocation), acquires CriticalSection lock, clears all existing resolvers
Creates a custom image provider from a JSON array that resolves ![alt](url)
markdown image links to actual images. Entries are either Path entries (vector
paths rendered to bitmap) or Image entries (loaded from the HISE image pool).
Registered at highest priority (EmbeddedPath).

Required setup:
  const var md = Content.createMarkdownRenderer();
  const var providers = [
      { "URL": "icon_check", "Type": "Path",
        "Data": Content.createPath(), "Colour": 0xFF00FF00 }
  ];
  md.setImageProvider(providers);

Dispatch/mechanics:
  new ScriptedImageProvider(mc, &renderer, data) -- iterates array, creates
    PathEntry or ImageEntry for each element based on "Type" property
  -> ScopedLock on action.lock
  -> renderer.clearResolvers() -- removes ALL existing resolvers (images + links)
  -> renderer.setImageProvider(newProvider)

Pair with:
  setText -- set markdown text containing ![alt](url) references that match provider URLs

Anti-patterns:
  - Calling setImageProvider() clears ALL existing resolvers (both image and link),
    not just the previous scripted provider. Only the new entries will resolve.
  - Passing a non-array value silently succeeds after clearing all resolvers,
    leaving the renderer with no image resolution capability.

Source:
  ScriptingGraphics.cpp  MarkdownObject::setImageProvider()
    -> new ScriptedImageProvider(mc, &renderer, data)
    -> renderer.clearResolvers()
    -> renderer.setImageProvider(newProvider)
  ScriptingGraphics.cpp:1394  ScriptedImageProvider inner class
  ScriptingGraphics.cpp:1428  PathEntry -- renders path as square bitmap
  ScriptingGraphics.cpp:1455  ImageEntry -- loads from pool via PoolReference
