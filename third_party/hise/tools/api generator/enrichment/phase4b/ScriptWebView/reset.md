ScriptWebView::reset() -> undefined

Thread safety: UNSAFE
Clears all cached resources and persistent call data from the underlying
WebViewData, but preserves the file structure (root directory and index file
settings). Use this to force a clean reload of web content without
re-specifying the content source.
Dispatch/mechanics:
  data->reset(false) -- resetFileStructure=false preserves root directory
  and index file, clears initScripts and cached resources
Pair with:
  setIndexFile -- typically called after reset during development iteration
  evaluate -- persistent scripts are cleared by reset
Source:
  ScriptingApiContent.cpp:6060  ScriptWebView::reset()
    -> data->reset(false)
