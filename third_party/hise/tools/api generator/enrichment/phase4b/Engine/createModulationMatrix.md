Engine::createModulationMatrix(String containerId) -> ScriptObject

Thread safety: UNSAFE -- heap allocation, processor lookup by string ID
Creates a modulation matrix using a GlobalModulatorContainer as the modulation source.
Used for building user-facing modulation assignment interfaces (drag-and-drop routing).
Anti-patterns:
  - Do NOT pass an ID that does not match a GlobalModulatorContainer -- the constructor
    will fail to find the container
Source:
  ScriptingApi.cpp  Engine::createModulationMatrix()
    -> new ScriptModulationMatrix(containerId) with processor tree lookup
