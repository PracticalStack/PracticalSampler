Links a script processor (created via `create()`) to an external `.js` file. The path is relative to the project's Scripts folder, using the same syntax as `include()` statements (e.g. `"{PROJECT_FOLDER}ConnectedScripts/MyScript.js"`).

The workflow for creating connected script processors is:

1. Write and test the script in any Script Processor
2. Right-click the script editor and choose "Save script to File"
3. In your Builder code, create a ScriptProcessor and pass its build index and the file reference to `connectToScript()`

Multiple processors can share the same script file - each gets its own state but uses the same logic. To apply changes to all connected processors after editing the script, use "Recompile all scripts".
