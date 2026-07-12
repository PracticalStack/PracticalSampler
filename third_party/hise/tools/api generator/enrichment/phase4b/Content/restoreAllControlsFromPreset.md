Content::restoreAllControlsFromPreset(String fileName) -> undefined

Thread safety: UNSAFE -- involves file I/O (reading XML from disk in backend) or ValueTree lookup (in frontend).
Restores all component values from a saved preset file. Accepts either an absolute path
or a path relative to the UserPresets folder. Components with saveInPreset=false are skipped.

Dispatch/mechanics:
  Frontend: reads from embedded ValueTree hierarchy via ProjectHandler
  Backend: reads from XML file on disk
  Matches processor ID against preset XML children
  Restores values for components with saveInPreset=true

Anti-patterns:
  - In frontend builds, the fileName is matched against the embedded FileName property
    in the ValueTree. A mismatch produces a "Preset ID not found" error.
  - If the processor ID does not match any child in the preset XML, a "Preset ID not
    found" error is thrown.

Source:
  ScriptingApiContent.cpp:8244  Content::restoreAllControlsFromPreset()
    -> USE_FRONTEND: embedded ValueTree lookup
    -> USE_BACKEND: XML file I/O from disk
