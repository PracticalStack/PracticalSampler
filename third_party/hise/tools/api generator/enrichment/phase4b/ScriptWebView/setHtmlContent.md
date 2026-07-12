ScriptWebView::setHtmlContent(String htmlCode) -> undefined

Thread safety: UNSAFE
Sets inline HTML content for the webview to render (Hardcoded content mode).
The entire page is specified as a string. For file-based content with external
CSS/JS resources, use setIndexFile() instead.
Pair with:
  setIndexFile -- alternative for file-based content with external resources
  reset -- clears cached content
Source:
  ScriptingApiContent.cpp:6005  ScriptWebView::setHtmlContent()
    -> data->setHtmlContent(htmlCode)
