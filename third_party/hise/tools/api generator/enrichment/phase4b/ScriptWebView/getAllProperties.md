ScriptWebView::getAllProperties() -> Array

Thread safety: UNSAFE
Returns an array of all active (non-deactivated) property IDs. Includes base
ScriptComponent properties and the four WebView-specific properties (enableCache,
enablePersistence, scaleFactorToZoom, enableDebugMode). Excludes deactivated
properties (macroControl, processorId, parameterId, tooltip, text, min, max,
defaultValue, etc.).
Source:
  ScriptingApiContent.cpp  ScriptComponent::getAllProperties() (base class)
