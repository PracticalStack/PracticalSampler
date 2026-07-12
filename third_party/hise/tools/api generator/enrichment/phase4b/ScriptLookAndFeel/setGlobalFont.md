ScriptLookAndFeel::setGlobalFont(String fontName, Double fontSize) -> undefined

Thread safety: UNSAFE
Sets the font used by all JUCE LookAndFeel font getter methods: alert window fonts,
popup menu fonts, combo box fonts, text button fonts, and dialog fonts. Does NOT affect
fonts used inside registerFunction() paint callbacks.

Dispatch/mechanics:
  MainController::getFontFromString(fontName, fontSize) -> stored as member Font f.
  All Laf font getters (getAlertWindowMessageFont, getTextButtonFont, getComboBoxFont,
  getPopupMenuFont, getAlertWindowFont, getAlertWindowTitleFont) return this font.

Anti-patterns:
  - Do NOT expect this to affect fonts inside registerFunction() paint callbacks --
    those are controlled by g.setFont() within each callback. setGlobalFont() only
    affects JUCE's built-in LookAndFeel methods.

Source:
  ScriptingGraphics.cpp:2553  ScriptedLookAndFeel::setGlobalFont()
    -> getMainController_()->getFontFromString(fontName, fontSize)
