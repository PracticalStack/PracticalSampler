Engine::getExtraDefinitionsInBackend() -> JSON

Thread safety: UNSAFE -- reads project settings, constructs DynamicObject
Returns platform-specific extra preprocessor definitions as a JSON object.
Returns empty object {} in compiled plugins.
Anti-patterns:
  - Do NOT expect compile-time definitions to be available in frontend builds --
    method returns empty object in exported plugins despite the name suggesting otherwise
Source:
  ScriptingApi.cpp  Engine::getExtraDefinitionsInBackend()
    -> tokenizes ExtraDefinitions setting -> builds DynamicObject
    -> [USE_BACKEND only, else returns empty object]
