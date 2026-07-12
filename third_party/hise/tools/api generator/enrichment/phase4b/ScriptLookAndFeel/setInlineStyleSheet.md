ScriptLookAndFeel::setInlineStyleSheet(String cssCode) -> undefined

Thread safety: UNSAFE
Parses and applies CSS code provided as a string, activating CSS rendering mode. Passing
an empty string disables CSS mode. Acquires a write lock on LookAndFeelRenderLock during
application, blocking concurrent paint calls.

Required setup:
  const var laf = Content.createLocalLookAndFeel();
  laf.setInlineStyleSheet("button { background: #333; border-radius: 5px; }");
  myButton.setLocalLookAndFeel(laf);

Dispatch/mechanics:
  Sets useInlineStyleSheet=true, generates file ID from hash ("inline_" + hash)
    -> setStyleSheetInternal(): parses via simple_css::Parser
    -> acquires write lock on LookAndFeelRenderLock
    -> clears graphics pool, replaces CSS collection
  If CSS contains syntax errors, a script error is thrown immediately.

Pair with:
  setStyleSheetProperty -- inject dynamic CSS variables (var(--name)) into the stylesheet
  setStyleSheet -- alternative: load CSS from external file instead of inline string
  registerFunction -- can combine CSS and script functions on same LAF (CombinedLaf)

Source:
  ScriptingGraphics.cpp:2558  ScriptedLookAndFeel::setInlineStyleSheet()
    -> setStyleSheetInternal() -> simple_css::Parser -> write lock on LookAndFeelRenderLock
