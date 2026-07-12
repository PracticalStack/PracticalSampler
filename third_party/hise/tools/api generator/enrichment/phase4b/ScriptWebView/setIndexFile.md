ScriptWebView::setIndexFile(ScriptObject indexFile) -> undefined

Thread safety: UNSAFE
Sets the HTML file to display. The file's parent directory becomes the root for
resolving relative resource paths (CSS, JS, images). Requires a File object --
string paths cause a script error.
Required setup:
  const var wv = Content.addWebView("WebView1", 0, 0);
  const var folder = FileSystem.getFolder(FileSystem.AudioFiles);
Dispatch/mechanics:
  Casts var to ScriptFile -> extracts parent directory as root
    -> data->setRootDirectory(parentDir)
    -> data->setIndexFile("/" + fileName)
Pair with:
  reset -- clear cached state before reloading during development
  setHtmlContent -- alternative for inline HTML without external resources
Anti-patterns:
  - Do NOT pass a string path -- must be a File object from the FileSystem API.
    Reports script error: "setIndexFile must be called with a file object"
Source:
  ScriptingApiContent.cpp:5988  ScriptWebView::setIndexFile()
    -> data->setRootDirectory(sf->f.getParentDirectory())
    -> data->setIndexFile("/" + sf->f.getFileName())
