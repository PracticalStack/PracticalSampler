## setEnableOpenGL

**Examples:**

```javascript:opengl-quality-selector
// Title: Graphics quality selector with OpenGL toggle
// Context: A ComboBox offers quality tiers. The first item disables
// OpenGL entirely; higher tiers enable it and persist the quality
// level to an XML settings file.

const var GRAPHICS_OFF = 1;   // ComboBox value for "Off"

const var settingsFile = FileSystem.getFolder(FileSystem.AppData).getChildFile("GraphicsSettings.xml");

var settingsData = settingsFile.loadFromXmlFile();

if(!isDefined(settingsData))
    settingsData = {};

if(!isDefined(settingsData.quality))
    settingsData["quality"] = 1; // default to medium

const var cmbGraphics = Content.getComponent("GraphicsQuality");

// Restore saved state on init
if(Settings.isOpenGLEnabled())
    cmbGraphics.setValue(settingsData.quality + 2);
else
    cmbGraphics.setValue(GRAPHICS_OFF);

inline function onGraphicsControl(component, value)
{
    if(value == GRAPHICS_OFF)
    {
        Settings.setEnableOpenGL(false);
        Engine.showMessageBox("OpenGL Deactivated",
            "Please reload this plugin to apply the change.", 0);
    }
    else
    {
        if(!Settings.isOpenGLEnabled())
        {
            Settings.setEnableOpenGL(true);
            Engine.showMessageBox("OpenGL Activated",
                "Please reload this plugin to apply the change.", 0);
        }

        settingsData.quality = parseInt(value - 2);
        settingsFile.writeAsXmlFile(settingsData, "GraphicsSettings");
    }
};

cmbGraphics.setControlCallback(onGraphicsControl);
```

```json:testMetadata:opengl-quality-selector
{
  "testable": false,
  "skipReason": "Requires UI component, file I/O for XML settings persistence, and plugin reload for OpenGL changes"
}
```

**Pitfalls:**
- Always show a reload message after toggling OpenGL. The change is deferred until the next interface rebuild - the user will see no visual difference until they reload the plugin.
- Persist the OpenGL state to a settings file (e.g., XML in AppData) so it survives plugin reloads. HISE does not automatically persist this setting for exported plugins.
