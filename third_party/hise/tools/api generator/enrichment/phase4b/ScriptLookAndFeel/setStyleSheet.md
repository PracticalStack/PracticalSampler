ScriptLookAndFeel::setStyleSheet(String fileName) -> undefined

Thread safety: UNSAFE
Loads and applies a CSS stylesheet from a file in the project's Scripts folder. In the
HISE IDE, the file is watched for live editing -- changes reflect immediately without
recompilation. In exported plugins, the CSS file is loaded from the embedded script
collection.

Required setup:
  const var laf = Content.createLocalLookAndFeel();
  laf.setStyleSheet("interface.css");
  myButton.setLocalLookAndFeel(laf);

Dispatch/mechanics:
  Stores filename -> loadStyleSheetFile():
    USE_BACKEND: loads from Scripts dir, creates default if missing, adds file watcher
    Frontend: loads from embedded script collection
    -> setStyleSheetInternal() -> simple_css::Parser -> write lock on LookAndFeelRenderLock

Pair with:
  setStyleSheetProperty -- inject dynamic CSS variables into the loaded stylesheet
  setInlineStyleSheet -- alternative: provide CSS as inline string instead of file
  registerFunction -- can combine CSS and script functions on same LAF (CombinedLaf)

Anti-patterns:
  - Do NOT omit the .css extension -- a script error is thrown if the file does not
    have a .css extension.
  - In the HISE IDE, a typo in the filename silently creates a new file with default
    content instead of producing an error. This auto-creation only happens in the IDE.

Source:
  ScriptingGraphics.cpp:2646  ScriptedLookAndFeel::setStyleSheet()
    -> loadStyleSheetFile() -> setStyleSheetInternal()
  ScriptingGraphics.cpp:2579  loadStyleSheetFile()
    -> USE_BACKEND: file watcher + external script file
    -> Frontend: getExternalScriptFromCollection()
