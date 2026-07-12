ScriptAudioWaveform::setStyleSheetClass(String classIds) -> undefined

Thread safety: UNSAFE
Sets CSS class selectors for this component. The component's type class is
automatically prepended (e.g. ".scriptaudiowaveform"). Pass space-separated
class names (e.g. ".active .highlighted").

Pair with:
  setLocalLookAndFeel -- must have a CSS-enabled LAF attached
  setStyleSheetProperty -- to set CSS variables

Source:
  ScriptingApiContent.cpp  ScriptComponent::setStyleSheetClass()
    -> creates ComponentStyleSheetProperties ValueTree if needed
    -> prepends type class -> stores selector string
