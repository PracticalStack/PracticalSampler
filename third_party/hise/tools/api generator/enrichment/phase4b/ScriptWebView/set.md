ScriptWebView::set(String propertyName, NotUndefined value) -> undefined

Thread safety: UNSAFE
Sets a component property to the given value. Reports a script error if the
property does not exist. During onInit, changes are applied without UI
notification; outside onInit, sends change notifications to update the UI. For
WebView-specific properties (enableCache, enablePersistence, scaleFactorToZoom,
enableDebugMode), the value is forwarded to the underlying WebViewData.

| Property | Description |
|----------|-------------|
| `x`, `y`, `width`, `height` | Position and size in pixels, relative to parent |
| `visible`, `enabled`, `locked` | Display and interaction state |
| `bgColour`, `itemColour`, `itemColour2`, `textColour` | Wrapper styling properties for the embedded web view component |
| `parentComponent` | Parent component for layout nesting |
| *`enableCache`*, *`enablePersistence`* | WebView data caching and persistent call replay behavior |
| *`scaleFactorToZoom`*, *`enableDebugMode`* | Browser zoom behavior and developer tools support |

Deactivated properties: `saveInPreset`, `macroControl`, `isPluginParameter`, `min`, `max`, `defaultValue`, `pluginParameterName`, `text`, `tooltip`, `useUndoManager`, `processorId`, `parameterId`, `isMetaParameter`, `linkedTo`, `automationID`.

Dispatch/mechanics:
  Base ScriptComponent::set() + override in setScriptObjectPropertyWithChangeMessage
  WebView properties forwarded: enableCache -> data->setEnableCache(),
  enablePersistence -> data->setUsePersistentCalls(), etc.
Pair with:
  get -- reads the property value back
Source:
  ScriptingApiContent.cpp  ScriptComponent::set() (base class)
  ScriptingApiContent.cpp:5883  ScriptWebView::setScriptObjectPropertyWithChangeMessage()
