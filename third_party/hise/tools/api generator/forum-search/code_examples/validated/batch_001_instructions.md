# Forum Example Analysis: batch_001.json

Analyze 73 examples from `/Users/christophhart/Development/hise_website_v2/hise_project_analysis/HISE/tools/api generator/forum-search/code_examples/batch_001.json` and generate validation metadata.

## Instructions

For each example listed below, generate a validation JSON object and apply it.
Use MCP tools (`query_scripting_api`, `list_module_types`, `list_ui_components`, 
`query_ui_property`) to look up unfamiliar APIs before generating setup code.

After analyzing all examples, write the enriched JSON to:
`/Users/christophhart/Development/hise_website_v2/hise_project_analysis/HISE/tools/api generator/forum-search/code_examples/validated/batch_001.json`

## System Prompt (analysis guidelines)

You are analyzing HiseScript code examples from the HISE audio framework forum. For each example, you must generate test metadata that will allow automated validation via the HISE runtime REST API.

## Your Task

For each code example, produce a JSON object with these fields:

```json
{
  "tier": <int 1-4>,
  "testable": <bool>,
  "skipReason": <string or null>,
  "setupCode": <string or null>,
  "testOnlyCode": <string or null>,
  "verifyScript": <array of verification objects or null>,
  "notes": <string or null>
}
```

## Tier Classification

- **Tier 1**: No setup needed. Self-contained code (pure functions, LAF definitions, code that creates its own components via Content.add*).
- **Tier 2**: Needs UI component creation. Code calls Content.getComponent("name") for components that don't exist. Setup must create them.
- **Tier 3**: Needs audio module creation AND possibly components. Code calls Synth.getEffect, Synth.getSampler, etc. for modules that don't exist.
- **Tier 4**: Depends on external resources (images, server, sample maps, expansions). Mark testable: false.

## Setup Code Rules

Setup code is **prepended** to the example code before compilation. It must:

1. Include `Content.makeFrontInterface(600, 400);` if the example doesn't already have it
2. Create any UI components the example references via Content.getComponent:
   - Knobs/Sliders: `Content.addKnob("name", x, y);`
   - Buttons: `Content.addButton("name", x, y);`
   - Panels: `Content.addPanel("name", x, y);`
   - Labels: `Content.addLabel("name", x, y);`
   - ComboBoxes: `Content.addComboBox("name", x, y);`
   - Tables: `Content.addTable("name", x, y);`
   - SliderPacks: `Content.addSliderPack("name", x, y);`
   - Viewports: `Content.addViewport("name", x, y);`
   - AudioWaveforms: `Content.addAudioWaveform("name", x, y);`
   - FloatingTiles: `Content.addFloatingTile("name", x, y);`
3. Infer component type from:
   - Name prefix (Knob*, knb* -> addKnob; Button*, btn* -> addButton; Panel*, pnl* -> addPanel)
   - Method usage (.setPaintRoutine -> Panel; .addItem -> ComboBox; .setRange -> Knob)
   - Default to addKnob if ambiguous

For Tier 3, modules must be created SEPARATELY before the example compiles (they persist across recompilations). Use this pattern in setupCode, but prefix module creation lines with `// MODULE_SETUP: ` so the validator can split them into a separate compilation step:
```
// MODULE_SETUP: Synth.addEffect("SimpleGain", "Simple Gain1", -1);
// MODULE_SETUP: Synth.addEffect("SimpleGain", "Simple Gain2", -1);
Content.makeFrontInterface(600, 400);
Content.addKnob("Knob1", 0, 0);
```

## Test-Only Code

Code appended after the example to trigger callbacks or exercise functionality. This code is compiled with the example but hidden from end users. Common patterns:
- `ComponentName.setValue(x); ComponentName.changed();` to trigger control callbacks
- Direct function calls for inline functions

## Verification Script

Array of verification steps. Each step is one of:

### REPL verification
```json
{"type": "REPL", "expression": "variableName", "value": expectedValue, "delay": 0}
```
- `expression`: any valid HiseScript expression accessible in onInit scope
- `value`: expected result (number, string, bool, or "undefined")
- `delay`: milliseconds to wait before checking (use 100-300 for async, 0 for sync)

### Log output verification
```json
{"type": "log-output", "values": ["expected", "log", "lines"]}
```
- Matches Console.print output in order

### Error expectation
```json
{"type": "expect-error", "errorMessage": "substring of expected error"}
```

## Important Guidelines

1. Only mark testable: false if truly untestable (external resources, modal dialogs like FileSystem.browse)
2. For LAF/paint routines: testable via compilation only. Set verifyScript to null —    successful compilation is sufficient.
3. For callback-only code (function onNoteOn, function onController): these are MIDI    callbacks that can't be triggered via REPL. Compilation test only.
4. Variables declared with `const var` in onInit scope are accessible via REPL.
5. Inline functions can be called from testOnlyCode.
6. REPL expressions must reference variables that exist after compilation.
7. `reg` variables persist globally. `var`/`const var` are script-scoped.
8. If the example uses Content.addKnob etc. to create its own components,    don't duplicate them in setup.
9. When computing expected REPL values, trace through the code carefully.    If the result depends on runtime state you can't predict, skip REPL verification.


## Examples to Analyze (indices: 0-72)

### [0] Save User Preset with Validation
URL: https://forum.hise.audio/topic/7486
Tags: forum, Scripting, expert, solved
Mechanical tier: 1
Description: FileSystem.browse and Engine.saveUserPreset used to implement a save-as dialog for user presets. Validates the chosen path is within the UserPresets folder and inside a subfolder before saving.
```javascript
inline function savePresetAs()
	{
		FileSystem.browse(FileSystem.getFolder(FileSystem.UserPresets), true, "*.preset", function(f)
		{
			var filePath = f.toString(File.FullPath);
			var parentDirectory = f.getParentDirectory().toString(File.FullPath);
			var userPresetsDirectory = FileSystem.getFolder(FileSystem.UserPresets).toString(File.FullPath);
			
			if (parentDirectory == userPresetsDirectory)
				return Engine.showMessageBox("Invalid Location", "Please choose a sub folder to store the preset.", 0);
				
			if (!filePath.contains(userPresetsDirectory))
				return Engine.showMessageBox("Invalid Location", "Please save the preset within the User Presets folder.", 0);

			Engine.saveUserPreset(f.toString(File.FullPath).replace(".preset"));
		});
	}
```

### [1] Component Lookup Map from getAllComponents
URL: https://forum.hise.audio/topic/14175
Tags: forum, Scripting, expert
Mechanical tier: 1
Description: Content.getAllComponents with an empty prefix builds a dictionary keyed by component ID for O(1) lookups. Useful alternative to individual Content.getComponent calls when many components need referencing.
```javascript
const comp = {};

for (x in Content.getAllComponents(""))
 comp[x.getId()] = x;
```

### [2] Batch Component Callback with getAllComponents
URL: https://forum.hise.audio/topic/13791
Tags: forum, Scripting, trusted, solved
Mechanical tier: 1
Description: Synth.getAllEffects and Content.getAllComponents with a shared prefix to batch-wire knobs to SimpleGain effects. Compact alternative to index-based loops using prefix matching.
```javascript
Content.makeFrontInterface(600, 400);

const var TheGains = Synth.getAllEffects("Simple Gain");
const var SampleVolumeKnobs = Content.getAllComponents("knbVolume");

for (k in SampleVolumeKnobs)
	k.setControlCallback(onSampleVolumeKnobsControl);

inline function onSampleVolumeKnobsControl(component, value)
{
	local idx = SampleVolumeKnobs.indexOf(component);
	TheGains[idx].setAttribute(0, value);
}
```

### [3] Disable Shift-Click Knob Text Input
URL: https://forum.hise.audio/topic/2998
Tags: forum, Scripting, trusted
Mechanical tier: 2
Description: Component modifiers API used to disable text input and double-click reset on knobs. createModifiers and setModifiers with the disabled flag prevent shift-click editing and default-value reset.
```javascript
Content.makeFrontInterface(600, 400);

const var knobs = [Content.getComponent("Knob1"),
	Content.getComponent("Knob2"),
	Content.getComponent("Knob3"),
	Content.getComponent("Knob4"),
	Content.getComponent("Knob5"),
	Content.getComponent("Knob6")];

const var mods = [];

for (x in knobs)
	mods.push(x.createModifiers());

for (i = 0; i < knobs.length; i++)
{
	knobs[i].setModifiers(mods[i].TextInput, [mods[i].disabled, mods[i].disabled]);
	knobs[i].setModifiers(mods[i].ResetToDefault, [mods[i].disabled, mods[i].disabled]);
}
```

### [4] Flipped FFT Spectrum Display
URL: https://forum.hise.audio/topic/13674
Tags: forum, Scripting, trusted, solved
Mechanical tier: 2
Description: Custom FFT visualization using Graphics.flip to mirror the spectrum path vertically with gradient fills and layered rendering. Uses beginLayer/endLayer for composited drawing of mirrored FFT paths.
```javascript
const var Panel1 = Content.getComponent("Panel1");

Panel1.setPaintRoutine(function(g)
{
	var a = this.getLocalBounds(0);
	var path2 = Content.createPath();
	path2 = obj.path;
	g.flip(false, a);
	g.setGradientFill([Colours.withAlpha(Colours.antiquewhite, 0.5), a[2] / 2, 0, Colours.withAlpha(Colours.black, 1.0), a[2] / 2, 250]);
	g.fillPath(path2, obj.pathArea);

	g.beginLayer(false);
	g.flip(false, a);
	g.setGradientFill([Colours.withAlpha(Colours.antiquewhite, 1.0), a[2] / 2, 0, Colours.withAlpha(Colours.black, 1.0), a[2] / 2, 250]);
	g.fillPath(obj.path, obj.pathArea);
	g.endLayer();
});
```

### [5] DllTimer and DllAsyncUpdater for Third-Party Nodes
URL: https://forum.hise.audio/topic/13600
Tags: forum, Scripting, author
Mechanical tier: 1
Description: hise::DllTimer and hise::DllAsyncUpdater as replacements for juce::Timer and juce::AsyncUpdater in third-party C++ nodes. Required because DLL processes cannot use JUCE timers directly.
```javascript
template <int NV> my_node: public juce::Timer // nope
{

};

template <int NV> my_node: public hise::DllTimer // this will work
{
 my_node()
 {
 startTimer(30);
 }

 void timerCallback() override { ... }
};

template <int NV> my_node: public juce::AsyncUpdater // nope
{

};

template <int NV> my_node: public hise::DllAsyncUpdater // this will work
{
 my_node()
 {
 
 }

 void process(ProcessData<2>& data) {
 triggerAsyncUpdate();
 }

 void handleAsyncUpdate() override {}
};
```

### [6] Broadcaster Mouse Event Listener
URL: https://forum.hise.audio/topic/13388
Tags: forum, Scripting, trusted
Mechanical tier: 2
Description: Engine.createBroadcaster with attachToComponentMouseEvents to track mouse drag on a FloatingTile. Converts vertical drag distance to a normalized 0-1 range using Math.range.
```javascript
const var FloatingTile1 = Content.getComponent("FloatingTile1");

const var Waterfall_BC = Engine.createBroadcaster({
 "id": "Waterfall_BC",
 "args": ["component", "event"],
 "tags": []
});

Waterfall_BC.attachToComponentMouseEvents(["FloatingTile1"], "Clicks, Hover & Dragging", "");

Waterfall_BC.addListener("", "md", function(component, event)
{
	if (event.drag)
		Console.print(Math.range(event.dragY / -component.getHeight(), 0.0, 1.0));
	
});
```

### [7] Copy MIDI File to AppData
URL: https://forum.hise.audio/topic/13354
Tags: forum, Scripting, expert, solved
Mechanical tier: 1
Description: File.copy used to duplicate a MIDI file into the AppData folder. Extracts the filename from the source File object and constructs a destination path via FileSystem.getFolder and getChildFile.
```javascript
inline function copyMidiFile(original)
{
 local filename = original.toString(original.Filename);
 local newFile = FileSystem.getFolder(FileSystem.AppData).getChildFile(filename);
 original.copy(newFile);
}
```

### [8] Auto-Load All Fonts from Folder
URL: https://forum.hise.audio/topic/13352
Tags: forum, Scripting, trusted
Mechanical tier: 4
Description: FileSystem.findFiles scans the Fonts folder for TTF/OTF files and loads each via Engine.loadFontAs. Strips the '-Regular' suffix from font names for cleaner references while preserving style suffixes like Bold and Italic.
```javascript
// this will load all of your fonts which are stored in the projectFolder/Images/Fonts
inline function loadAllFontsFromProjectFolder()
{
	local appDataFolder = FileSystem.getFolder(FileSystem.AppData);
	local fontsFolder = appDataFolder.getChildFile("Fonts");
	local fontFiles = FileSystem.findFiles(fontsFolder, "*.ttf;*.otf", false);

	for (file in fontFiles)
	{
		local fontPath = file.toString(File.FullPath);
		local fontName = file.toString(File.Filename);
		local targetName = "";

		// if the file name suffix is "-Regular", loadAs will use just the prefix
		if (fontName.endsWith("-Regular.ttf") || fontName.endsWith("-Regular.otf"))
			targetName = fontName.substring(0, fontName.lastIndexOf("-Regular."));
		else if (fontName.endsWith(".ttf") || fontName.endsWith(".otf"))
			targetName = fontName.substring(0, fontName.lastIndexOf("."));

		Engine.loadFontAs(fontPath, targetName);
	}
}
```

### [9] Offline FFT Spectrum Generation and Display
URL: https://forum.hise.audio/topic/13231
Tags: forum, Scripting, author
Mechanical tier: 1
Description: Engine.createFFT with setEnableSpectrum2D to generate and render a 2D spectrum from a synthesized buffer. Configures FFT parameters (size, oversampling, gamma, colour scheme) and paints the result with drawFFTSpectrum on a Panel.
```javascript
Content.makeFrontInterface(600, 600);

// Create a buffer with a second length
const var b = Buffer.create(44100.0);

// fill it up with some random signal - two sine sweeps and some noise
reg uptime = 0.0;
reg delta = 0.01;

for(s in b)
{
	s = Math.sin(uptime);
	s += 0.5 * Math.sin(uptime * 2.0);
	s += 0.3 * Math.random();
	
	uptime += delta;
	delta += 0.00003;
}

// Create the FFT object that is used to create the spectrum
const var fft = Engine.createFFT();

// Enable the spectrum generation
fft.setEnableSpectrum2D(true);

// Setup the FFT processing (FFT size & channel count)
fft.prepare(1024, 1);

// Fetch the spectrum options
const var options = fft.getSpectrum2DParameters();

// Dump that
Console.print(trace(options));

// change whatever property you want.
options.ColourScheme = 2;
options.Oversampling = 8;
options.Gamma = 30;
options.ResamplingQuality = "High";

// Send it back
fft.setSpectrum2DParameters(options);

// process the buffer. This creates the spectrum
// that can then be painted
fft.process(b);

// create a panel
const var p = Content.addPanel("P1", 0, 0);

p.set("width", 600);
p.set("height", 600);

p.setPaintRoutine(function(g)
{
	// This function paints the FFT spectrum on the panel
	g.drawFFTSpectrum(fft, this.getLocalBounds(0));
});
```

### [10] Browse and Load Audio File as Buffer
URL: https://forum.hise.audio/topic/13231
Tags: forum, Scripting, trusted
Mechanical tier: 2
Description: FileSystem.browse with loadAsAudioFile to let the user pick an audio file from Desktop and load it into an array of buffers. Button callback triggers the file dialog with a completion handler.
```javascript
inline function onButton1Control(component, value)
{
	if (value)
	{
		FileSystem.browse(FileSystem.Desktop, false, "", function(result)
		{
			arrayOfBuffers = result.loadAsAudioFile();
			
			// do fft with this array of buffers
		});
	}
};

Content.getComponent("Button1").setControlCallback(onButton1Control);
```

### [11] Array-Indexed Knob Step Values
URL: https://forum.hise.audio/topic/13189
Tags: forum, Scripting, expert
Mechanical tier: 2
Description: Lookup array maps discrete knob positions to arbitrary step values. The knob value serves as an array index, enabling non-linear value mappings like musical intervals or power-of-two steps.
```javascript
Content.makeFrontInterface(600, 400);

const var steps = [1, 3, 5, 7, 9];

inline function onKnobControl(component, value)
{
	local stepValue = steps[value];
	Console.print(stepValue);
}

Content.getComponent("Knob1").setControlCallback(onKnobControl);
```

### [12] Mouse Drag Detection Panel Overlay
URL: https://forum.hise.audio/topic/13085
Tags: forum, Scripting, trusted
Mechanical tier: 2
Description: setMouseCallback on a detection panel tracks drag state to show/hide a blocking overlay. Compares mouse coordinates against panel bounds and previous position to distinguish dragging from hovering.
```javascript
const var pnl_DETECT = Content.getComponent("pnl_DETECT");
const var tile_DRAGROP = Content.getComponent("tile_DRAGROP");
const var pnl_BLOCK = Content.getComponent("pnl_BLOCK");

pnl_DETECT.setMouseCallback(function(event)
{
 var w = this.getWidth();
 var h = this.getHeight();
 var isMouseOver = (event.x >= 0 && event.x <= w && event.y >= 0 && event.y <= h);

 var isDragging = event.drag || event.mouseDown || (event.x != this.data.lastX || event.y != this.data.lastY);
 
 this.data.lastX = event.x;
 this.data.lastY = event.y;
 
 if (isMouseOver && isDragging)
 {
 pnl_BLOCK.showControl(false);
 }
 else
 {
 pnl_BLOCK.showControl(true);
 }
});
```

### [13] Macro Knob Controlling Multiple Parameters
URL: https://forum.hise.audio/topic/5385
Tags: forum, Scripting, trusted
Mechanical tier: 2
Description: Normalised-to-range helper maps a single macro knob to multiple parameters with different ranges. Uses setValue/changed pattern to programmatically update knob values from a control callback.
```javascript
Content.makeFrontInterface(600, 400);

const var LF = Content.getComponent("LF");
const var HF = Content.getComponent("HF");
const var SATURATION = Content.getComponent("SATURATION");
const var WIDTH = Content.getComponent("WIDTH");

inline function normalisedToRange(normalisedValue, min, max)
{
	return min + normalisedValue * (max - min);
}

inline function onMAINControl(component, value)
{
	local v1 = normalisedToRange(value, 0, 7);
	local v2 = normalisedToRange(value, 0, 7);
	local v3 = normalisedToRange(value, 0, 1);
	local v4 = normalisedToRange(value, 100, 170);

	LF.setValue(v1);
	HF.setValue(v2);
	SATURATION.setValue(v3);
	WIDTH.setValue(v4);

	LF.changed();
	HF.changed();
	SATURATION.changed();
	WIDTH.changed();
}

Content.getComponent("MAIN").setControlCallback(onMAINControl);
```

### [14] Read and Write JSON File
URL: https://forum.hise.audio/topic/13022
Tags: forum, Scripting, expert, solved
Mechanical tier: 1
Description: FileSystem.getFolder with getChildFile, writeObject, and loadAsObject for persistent JSON storage. Includes save and load utility functions with file-existence validation before reading.
```javascript
// Create an object
const myObj = {"name": "Dave", "age": 38, "country": "England"};

// Save object to a file on desktop
inline function saveObject(obj)
{
 local f = FileSystem.getFolder(FileSystem.Desktop).getChildFile("myFile.json"); 
 f.writeObject(obj);
}

// Load the file 
inline function loadObject()
{
 local f = FileSystem.getFolder(FileSystem.Desktop).getChildFile("myFile.json"); 
 
 if (!f.isFile())
 return {}; // Invalid file, return empty object

 return f.loadAsObject(); // What you will have here is the same as the object you started with
}
```

### [15] CSS Rotate Keyboard Floating Tile
URL: https://forum.hise.audio/topic/12942
Tags: forum, Scripting, trusted, solved
Mechanical tier: 2
Description: setInlineStyleSheet with CSS transform rotate(90deg) applied to a keyboard FloatingTile via setLocalLookAndFeel. Demonstrates CSS transforms on HISE floating tile components.
```javascript
const var ft_Keyboard = Content.getComponent("ft_Keyboard");
const var laf_Keyboard = Content.createLocalLookAndFeel();

ft_Keyboard.setLocalLookAndFeel(laf_Keyboard);

laf_Keyboard.setInlineStyleSheet("
.keyboard 
{ 
	background: red; 
	transform: rotate(90deg);
}
");
```

### [16] PNG Linear Slider LAF
URL: https://forum.hise.audio/topic/12925
Tags: forum, Scripting, trusted
Mechanical tier: 4
Description: LAF.loadImage and registerFunction drawLinearSlider for a custom image-based vertical slider. Calculates thumb position from valueNormalized and draws background and cap images at computed offsets.
```javascript
const var LAF = Content.createLocalLookAndFeel();
const var SliderBackground = LAF.loadImage("{PROJECT_FOLDER}SliderBackground.png", "Background");
const var SliderCap = LAF.loadImage("{PROJECT_FOLDER}SliderCap.png", "SliderCap");
const var Knob1 = Content.getComponent("Knob1");

LAF.registerFunction("drawLinearSlider", function(g, obj)
{
		var a = obj.area;

		var h = a[3] / 13;
		var y = a[3] - a[3] * obj.valueNormalized - (h + 10) + (h + 10) * obj.valueNormalized;
			
		g.drawImage("Background", [0,0, 80, 560], 0, 0);		
		g.drawImage("SliderCap", [-10, y, a[2] + 20, h + 20],0 , 0);
});
Content.getComponent("Knob1").setLocalLookAndFeel(LAF);
```

### [17] Dynamic Slider Creation Loop
URL: https://forum.hise.audio/topic/12903
Tags: forum, Scripting, trusted
Mechanical tier: 1
Description: Content.addKnob in a loop to dynamically create multiple sliders with indexed names. Returns an array of component references and extracts IDs via the Array.map function.
```javascript
// a loop to create sliders
inline function createSliders(stringName, numOfSliders)
{
	local sliders = [];

	for (i = 0; i < numOfSliders; i++)
	{
		// this appends the index to the slider name: "stringName + i"
		local slider = Content.addKnob(stringName + i, 50 + i * 125, 50);
		// this stores the component object inside an array
		sliders[i] = slider;
		slider.set("style", "vertical");
	}
	return sliders;
}

// call the function to create the sliders
const mySliders = createSliders("MySlider", 4);
// a separate array for the names ("Id")
const mySlidersIdArray = mySliders.map(function(element){return element.getId();});

Console.print("mySlidersIdArray: " + trace(mySlidersIdArray));
```

### [18] CSS Preset Browser Hover Styling
URL: https://forum.hise.audio/topic/12893
Tags: forum, Scripting, author
Mechanical tier: 2
Description: setInlineStyleSheet using HTML table row selectors (tr, tr:hover) to style preset browser list items. Applies background colour changes on hover via CSS pseudo-classes on a FloatingTile.
```javascript
const var laf = Content.createLocalLookAndFeel();

Content.getComponent("FloatingTile1").setLocalLookAndFeel(laf);

laf.setInlineStyleSheet("

tr /** HTML table row => preset browser list item */
{
	background: red;
}

tr:hover
{
	background: green;
}

")
```

### [19] Preset Change Callback Label Update
URL: https://forum.hise.audio/topic/12873
Tags: forum, Scripting
Mechanical tier: 2
Description: UserPresetHandler.setPostCallback updates a label with Engine.getCurrentUserPresetName after each preset load. Simple pattern for displaying the active user preset name in a custom UI element.
```javascript
//PresetDisplay
const var PresetNameLabel = Content.getComponent("PresetNameLabel");
UserPresetHandler.setPostCallback(function()
{
	PresetNameLabel.set("text", Engine.getCurrentUserPresetName());
});
```

### [20] CSS-Styled Label with Selection
URL: https://forum.hise.audio/topic/12796
Tags: forum, Scripting, trusted
Mechanical tier: 2
Description: setInlineStyleSheet for a Label component with CSS selectors for default, input, and ::selection states. Configures letter-spacing, padding, caret colour, text shadow, and selection highlight via inline CSS.
```javascript
Content.makeFrontInterface(200, 50);

const var Label1 = Content.getComponent("Label1");

const var lblCss = Content.createLocalLookAndFeel();

Label1.set("text", "CSS Label");

// Email/Key Label Laf
lblCss.setInlineStyleSheet("
*
{
	letter-spacing: 2px;
	font-weight: regular;
}

/** Render the default appearance. */
label
{
	background-color: var(--bgColour);
	color: var(--textColour);	
	border-radius: 5px;
	padding-left: 50px;
	padding-right: 20px;
	text-shadow: 2px 2px 5px rgba(0,0,0,0.3);
}

/** If you edit the text, it will use this selector. */
input
{

	text-align: left;
	padding-top: 0.5px;
	padding-left: 50px;
	padding-right: 20px;
	caret-color: white;
	font-weight: bold;
}

/** Style the text selection with this selector. */
::selection
{
	background: #50FFFFFF;
	color: white;
}
");

Label1.setLocalLookAndFeel(lblCss);
```

### [21] Configurable Grid Overlay Panel
URL: https://forum.hise.audio/topic/12785
Tags: forum, Scripting, trusted
Mechanical tier: 2
Description: Panel setPaintRoutine draws a configurable grid with adjustable line count, thickness, and colours. Useful as a layout helper during UI development for aligning components to a visual grid.
```javascript
const var Grid1 = Content.getComponent("Grid1");

Grid1.setPaintRoutine(function(g)
{
	var a = this.getLocalBounds(0);

	var NumBarsX = 10;
	var NumBarsY = 10;
	var BarHeight = 1;
	var BarWidth = 1;

	var BarColour = Colours.withAlpha(Colours.black, 0.5);
	var BackgroundColour = Colours.withAlpha(Colours.pink, 0.5);

	var spacerY = a[3] / NumBarsY;
	var spacerX = a[2] / NumBarsX;

	g.fillAll(BackgroundColour);
	g.setFont("ArialBOLD", 12);
	g.drawAlignedText("GRIDHELPER1.0", [5, 5, 100, 5], "left");

	for (i = 0; i <= NumBarsX; i++)
	{
		var x = i * spacerX - (BarWidth / 2);
		g.setColour(BarColour);
		g.fillRect([x, 0, BarWidth, a[3]]);
	}

	for (i = 0; i <= NumBarsY; i++)
	{
		var y = i * spacerY - (BarHeight / 2);
		g.setColour(BarColour);
		g.fillRect([0, y, a[2], BarHeight]);
	}
});
```

### [22] Find Child Components by Parent
URL: https://forum.hise.audio/topic/12584
Tags: forum, Scripting, expert
Mechanical tier: 1
Description: Content.getAllComponents with parentComponent property check to find all children of a specific panel. Iterates all components and filters by their parentComponent property string.
```javascript
for (c in Content.getAllComponents(""))
{
	if (c.get("parentComponent") == "Panel1")
		Console.print(c.getId() + " is a child of the panel");
	else
		Console.print(c.getId() + " is not a child of the panel");
}
```

### [23] Level-Driven Image Alpha with Timer
URL: https://forum.hise.audio/topic/12557
Tags: forum, Scripting, trusted, solved
Mechanical tier: 3
Description: Engine.createTimerObject polling Synth.getEffect getCurrentLevel to drive image opacity. Converts gain factor to decibels with Engine.getDecibelsForGainFactor and maps the value to setAlpha for visual feedback.
```javascript
const var Image8 = Content.getComponent("Image8");
const var t4 = Engine.createTimerObject();
const var InGain = Synth.getEffect("InGain");
t4.setTimerCallback(function()
{	
	var v4 = InGain.getCurrentLevel([0,1]);
	v4 = Engine.getDecibelsForGainFactor(v4);
	if (v4 > -12)
	{
	Image8.setAlpha(v4 * v4 / 101);		
	}
	
});

t4.startTimer(11);
```

### [24] Sampler Timestretch Configuration
URL: https://forum.hise.audio/topic/11966
Tags: forum, Scripting, expert
Mechanical tier: 3
Description: Synth.getSampler with getTimestretchOptions and setTimestretchOptions to modify timestretch behaviour. Fetches the current options object, modifies SkipLatency, and applies the updated settings back to the sampler.
```javascript
const var Sampler1 = Synth.getSampler("Sampler1");

// fetch the current state
const var obj = Sampler1.getTimestretchOptions();

// Apply your changes
obj.SkipLatency = false;

// Update the options
Sampler1.setTimestretchOptions(obj);
```

### [25] Hybrid Script and CSS ComboBox Styling
URL: https://forum.hise.audio/topic/12043
Tags: forum, Scripting, author, solved
Mechanical tier: 2
Description: registerFunction drawComboBox for the main combobox appearance combined with setInlineStyleSheet for popup menu CSS. Demonstrates how script LAF and CSS coexist with CSS taking precedence for popup menus.
```javascript
const var laf = Content.createLocalLookAndFeel();

// Combobox is styled by a function
laf.registerFunction("drawComboBox", function(g, obj)
{
	g.setColour(obj.bgColour);
	g.fillRect(obj.area);
	g.setFont("Comic Sans MS", 21);
	g.setColour(Colours.blue);
	g.drawAlignedText(obj.text, obj.area, "centred");
});

// Popup menu is styled by CSS
laf.setInlineStyleSheet("
.popup
{
	background: #333;
}

.popup-item
{
	background: transparent;
	color: #999;
	padding: 10px;
}

.popup-item:hover
{
	background: rgba(255,255,255, 0.2);
}

.popup-item:active
{
	color: white;
	font-weight: bold;
}
");

Content.getComponent("ComboBox1").setLocalLookAndFeel(laf);
```

### [26] Path Rotation by Angle
URL: https://forum.hise.audio/topic/12264
Tags: forum, Scripting, trusted
Mechanical tier: 1
Description: Content.createPath with manual rotation transform using Math.cos and Math.sin. Rotates an array of [x,y] points around the area centre by a given angle and builds a closed path from the result.
```javascript
inline function createRotatedPath(pathArray, area, angle)
{
	local rad = Math.toRadians(angle);
	local cosA = Math.cos(rad);
	local sinA = Math.sin(rad);

	local path = Content.createPath();

	local rotationCenterX = area[0] + (area[2] * 0.5);
	local rotationCenterY = area[1] + (area[3] * 0.5);

	for (i = 0; i < pathArray.length; i++)
	{
		local x = pathArray[i][0];
		local y = pathArray[i][1];

		local newX = (x - rotationCenterX) * cosA - (y - rotationCenterY) * sinA + rotationCenterX;
		local newY = (x - rotationCenterX) * sinA + (y - rotationCenterY) * cosA + rotationCenterY;

		if (i == 0)
			path.startNewSubPath(newX, newY);
		else
			path.lineTo(newX, newY);
	}

	path.closeSubPath();
	return path;
}
```

### [27] CSS Cursor Styling for Components
URL: https://forum.hise.audio/topic/10780
Tags: forum, Scripting, author
Mechanical tier: 2
Description: setInlineStyleSheet with CSS cursor property to set pointer cursor on buttons and text cursor on labels. Applies different cursor types per component type via CSS selectors and setLocalLookAndFeel.
```javascript
const var laf = Content.createLocalLookAndFeel();

laf.setInlineStyleSheet("

button
{
	cursor: pointer;
}

label
{
	background: #444;
	text-align: center;
	cursor: text;
}

");

Content.getComponent("Button2").setLocalLookAndFeel(laf);
Content.getComponent("Label1").setLocalLookAndFeel(laf);
```

### [28] Audio File Content Callback
URL: https://forum.hise.audio/topic/12129
Tags: forum, Scripting, expert, solved
Mechanical tier: 1
Description: Synth.getAudioSampleProcessor with getAudioFile and setContentCallback to respond when an AudioLoopPlayer's audio file content changes. Useful for triggering analysis or UI updates after file loading.
```javascript
const AudioLoopPlayer1 = Synth.getAudioSampleProcessor("Audio Loop Player1");
const af = AudioLoopPlayer1.getAudioFile(0);

af.setContentCallback(function()
{
 Console.print("HELLO WORLD");
});
```

### [29] Label Key Press Callback Setup
URL: https://forum.hise.audio/topic/11450
Tags: forum, Scripting, expert
Mechanical tier: 2
Description: setConsumedKeyPresses with 'all' and setKeyPressCallback on a Label to capture and log keyboard events. Starting point for implementing custom keyboard shortcuts on input components.
```javascript
const var Label1 = Content.getComponent("Label1");

Label1.setConsumedKeyPresses("all");

Label1.setKeyPressCallback(function(event)
{
	Console.print(trace(event));
});
```

### [30] Key Press Callback with Focus Detection
URL: https://forum.hise.audio/topic/11450
Tags: forum, Scripting, expert
Mechanical tier: 2
Description: setConsumedKeyPresses with a specific keyCode object to capture only the Escape key. setKeyPressCallback distinguishes focus changes from key presses via isFocusChange for targeted keyboard handling.
```javascript
const var Label1 = Content.getComponent("Label1");

Label1.setConsumedKeyPresses({keyCode: 27});

Label1.setKeyPressCallback(function(event)
{
	if (event.isFocusChange)
	{
		// Respond to focus changes here
		Console.print("FOCUS CHANGE");
		return;
	}
	
	// If we get to here, then the escape key (27) must have been pressed
	Console.print("ESC PRESSED!!!");
	
});
```

### [31] Sampler Selection Filter by Properties
URL: https://forum.hise.audio/topic/11921
Tags: forum, Scripting, expert
Mechanical tier: 3
Description: Synth.getSampler createSelectionWithFilter using Sampler.Root, Sampler.HiVel, and Sampler.RRGroup to query sample properties. Enables runtime access to sample metadata like SampleEnd for specific samples matching filter criteria.
```javascript
const var Sampler1 = Synth.getSampler("Sampler1");

const list = Sampler1.createSelectionWithFilter(function()
{
 return this.get(Sampler.Root) == 72 && this.get(Sampler.HiVel) > 20 && this.get(Sampler.RRGroup) == 1;
});

Console.print(list[0].get(Sampler.SampleEnd));
```

### [32] Monophonic Note Handling with Artificial Events
URL: https://forum.hise.audio/topic/11796
Tags: forum, Scripting, expert
Mechanical tier: 1
Description: Message.ignoreEvent with Synth.playNote and noteOffByEventId for monophonic voice management. Checks isArtificialEventActive before killing previous notes and applies noteOffDelayedByEventId for timed release.
```javascript
reg eventIds = Engine.createMidiList();

function onNoteOn()
{
	local n = Message.getNoteNumber();
	local v = Message.getVelocity();

	Message.ignoreEvent(true);

	if (Synth.isArtificialEventActive(eventIds.getValue(n)))
		Synth.noteOffByEventId(eventIds.getValue(n));

	eventIds.setValue(n, Synth.playNote(n, v));
	Synth.noteOffDelayedByEventId(eventIds.getValue(n), 1000);
}
```

### [33] Bitcrusher Staircase Visualization
URL: https://forum.hise.audio/topic/11733
Tags: forum, Scripting, trusted
Mechanical tier: 3
Description: Panel setPaintRoutine drawing a dynamic staircase pattern driven by a knob value. StepsKnob controls both the visual step count and Bitcrusher.BitDepth attribute, with repaintImmediately in the control callback for real-time feedback.
```javascript
const var Bitcrusher = Synth.getEffect("Bitcrusher");
const var BitcrusherPanel = Content.getComponent("BitcrusherPanel");
const var StepsKnob = Content.getComponent("StepsKnob");
StepsKnob.setRange(4, 16, 1);
StepsKnob.set("stepSize", 0.1);

BitcrusherPanel.setPaintRoutine(function(g) 
{
 var steps = StepsKnob.getValue(); // Get the number of steps from the knob
 var width = this.getWidth();
 var height = this.getHeight();
 var stepWidth = width / steps;
 var stepHeight = height / steps;
	
 g.fillAll(Colours.black);
	
 // Draw the staircase
 g.setColour(Colours.white);
 for (i = 0; i < steps; i++) 
 {
 var x1 = i * stepWidth;
 var x2 = (i + 1) * stepWidth;
 var y = height - i * stepHeight;
 g.drawLine(x1, x2, y, y, 2); // Horizontal line
 if (i < steps - 1) 
 {
 g.drawLine(x2, x2, y, height - (i + 1) * stepHeight, 2); // Vertical line
 }
 }
});

// Repaint the panel based on Knob value, attach it to the FX
inline function onStepsKnobControl(component, value)
{
	BitcrusherPanel.repaintImmediately();
	Bitcrusher.setAttribute(Bitcrusher.BitDepth, value);
};

Content.getComponent("StepsKnob").setControlCallback(onStepsKnobControl);
```

### [34] LayoutBuilder Dynamic UI Factory
URL: https://forum.hise.audio/topic/11321
Tags: forum, Scripting, expert
Mechanical tier: 2
Description: Recursive stack-based component builder that processes a JSON tree of component definitions. Uses Content.addKnob/addButton/addPanel etc. to create components dynamically and applies all properties from the JSON, enabling portable UI layouts between projects.
```javascript
/*
* Author: David Healey
* License: CC0
* Last updated: 08/12/2024
*/

namespace LayoutBuilder
{
	const data = [];

	inline function add(arr: Array)
	{
 data.concat(arr);
	}

	inline function process()
	{
		local stack = data.clone();
		
		while (stack.length > 0)
		{
			//local node = stack.shift();
			local node = stack.pop();
			
			if (!isDefined(node.type) || !isDefined(node.id))
				continue;

			addComponent(node.id, node);

			if (!isDefined(node.childComponents))
				continue;

			for (x in node.childComponents)
				x.parentComponent = node.id;

			for (i = node.childComponents.length - 1; i >= 0; i--)
			{
				stack.push(node.childComponents[i]);
			}

			//stack.concat(node.childComponents);
		}
	}
	
	inline function: object addComponent(id: string, properties: JSON)
	{
		local c = Content.getComponent(id);

		if (!isDefined(c))
		{
			local type = properties.type.replace("Scripted").replace("Script");

			switch (type)
			{
				case "Slider": c = Content.addKnob(id, properties.x, properties.y); break;
				case "Button": c = Content.addButton(id, properties.x, properties.y); break;
				case "Table": c = Content.addTable(id, properties.x, properties.y); break;
				case "ComboBox": c = Content.addComboBox(id, properties.x, properties.y); break;
				case "Label": c = Content.addLabel(id, properties.x, properties.y); break;
				case "Image": c = Content.addImage(id, properties.x, properties.y); break;
				case "Viewport": c = Content.addViewport(id, properties.x, properties.y); break;
				case "Panel": c = Content.addPanel(id, properties.x, properties.y); break;
				case "AudioWaveform": c = Content.addAudioWaveform(id, properties.x, properties.y); break;
				case "SliderPack": c = Content.addSliderPack(id, properties.x, properties.y); break;
				case "WebView": c = Content.addWebView(id, properties.x, properties.y); break;
				case "FloatingTile": c = Content.addFloatingTile(id, properties.x, properties.y); break;
			}
		}
		
		local allProperties = c.getAllProperties();
	
		for (x in properties)
		{
			if (!allProperties.contains(x) || ["id"].contains(x))
				continue;

			c.set(x, properties[x]);
		}

		return c;
	}
```

### [35] TransportHandler Beat Callback
URL: https://forum.hise.audio/topic/11064
Tags: forum, Scripting, trusted
Mechanical tier: 1
Description: TransportHandler onBeat callback using isNewBar flag to trigger different notes on bar boundaries. Synth.addNoteOn with noteOffDelayedByEventId creates timed note events synchronized to the host transport.
```javascript
inline function onBeat(beat, isNewBar) 
{
 if(isNewBar)
 {
	 Console.print("new bar");
 }
 local id = Synth.addNoteOn(1, 80 + isNewBar * 12, 127, 0);
 Synth.noteOffDelayedByEventId(id, 800);
}
```

### [36] Arpeggiator SliderPack Direct Access
URL: https://forum.hise.audio/topic/8573
Tags: forum, Scripting, trusted
Mechanical tier: 2
Description: Synth.getSliderPackProcessor with getSliderPack and ScriptSliderPack.referToData to bind UI slider packs directly to Arpeggiator data. Enables direct manipulation of arpeggiator velocity, length, and semitone packs.
```javascript
const var Arpeggiator1 = Synth.getSliderPackProcessor("Arpeggiator1");

//	Use sliderpack components for direct access to the arp
//	you can hide them if you don't want them in your ui
//	it's possible to access them anyway
const var SLPs = [Content.getComponent("SLP1"),
 Content.getComponent("SLP2"),
 Content.getComponent("SLP3")];

for (i = 0; i < 3; i++)
	SLPs[i].referToData(Arpeggiator1.getSliderPack(i));
```

### [37] Broadcaster Global Fallback Pattern
URL: https://forum.hise.audio/topic/9746
Tags: forum, Scripting, author, solved
Mechanical tier: 1
Description: isDefined check on a global namespace broadcaster with fallback to Engine.createBroadcaster for modular code. Allows namespaces to share a broadcaster if it exists globally or create a local one for standalone use.
```javascript
namespace Something
{
 const var someBroadcaster = isDefined(DataGlobal.someBroadcaster) ?
 DataGlobal.someBroadcaster : 
 Engine.createBroadcaster({});
}
```

### [38] JWT License Activation with Server API
URL: https://forum.hise.audio/topic/10284
Tags: forum, Scripting
Mechanical tier: 4
Description: Server.setBaseURL and Server.callWithPOST for a complete JWT authentication and license activation workflow. Chains token retrieval, validation via Server.setHttpHeader with Bearer token, and WooCommerce license activation through sequential POST requests.
```javascript
// Server Address
Server.setBaseURL("

// Credentials for authentication
const var credentials = 
{
 "username": "you.com",
 "password": "yourpassword"
}

// Relevant references
const var authUrl = "/wp-json/jwt-auth/v1/token";
const var validateUrl = "/wp-json/jwt-auth/v1/token/validate";
const var activateUrl = "/wp-json/wclm/v3/activate";

reg jwtToken = "";

// Some debug stuff
Console.clear();
if (Server.isOnline()) Console.print("Server is Online!" + "\n");
Console.print("Authorization URL: " + authUrl);
Console.print("Validate URL: " + validateUrl);
Console.print("Activate URL: " + activateUrl + "\n");

// Authenticate and retrieve token
inline function authenticateUser() 
{
 Console.print("Starting authentication process...");
 
 Server.callWithPOST(authUrl, credentials, printResponse);
};

inline function printResponse(status, response) 
{
 Console.print("Received response: " + JSON.stringify(response));
 
 if (response.token != "") 
 {
 jwtToken = response.token;
 
 validateToken();
 } 
 else 
 {
 Console.print("Authentication failed: " + response.message);
 }
};

// Validate the JWT token
inline function validateToken() 
{
	Console.print("atempting to validate");

 if (jwtToken != "") 
 {
 Server.setHttpHeader("Authorization: Bearer " + jwtToken);

 Server.callWithPOST(validateUrl, {}, function(status, response) 
 {
 Console.print("Validation response: " + JSON.stringify(response));

 if (response.code == "jwt_auth_valid_token") 
 {
 Console.print("Token is valid!");
 
 activateLicense();
 } 
 else 
 {
 Console.print("Token validation failed: " + response.message);
 }
 });
 } 
 else 
 {
 Console.print("No JWT token found.");
 }
}

// License activation details
const var licenseData = 
{
 "license_key": "your-prod-key-lic"
};

// Activate the license
inline function activateLicense() 
{
 Console.print("Starting license activation..." + licenseData.license_key);

 // Set the Authorization header with the JWT token
 Server.setHttpHeader("Authorization: Bearer " + jwtToken);

 // Send the POST request to activate the license
 Server.callWithPOST(activateUrl, licenseData, handleActivationResponse);

};

// Function to handle the response from license activation
inline function handleActivationResponse(status, response)
{
	Console.print(response.signature);
	
 if (response["response"]["result"] == "success") 
 {
 Console.print(response["response"]["message"] + "!");
 } 
 else 
 {
 Console.print(response["response"]["message"] + "!");
 }
};

// Example: Trigger authentication when a button is clicked
inline function onButton1Control(component, value) 
{
 if (value) authenticateUser();
}
Content.getComponent("Button1").setControlCallback(onButton1Control);
```

### [39] Hardcode Development Audio Setup
URL: https://forum.hise.audio/topic/9722
Tags: forum, Scripting, trusted
Mechanical tier: 1
Description: Settings.getAvailableDeviceNames, setAudioDevice, setBufferSize, and toggleMidiInput inside an Engine.isHISE guard to auto-configure audio and MIDI devices during development. Prevents repeated manual setup after crashes or hardware changes.
```javascript
// Default HISE Audio Setup
if (Engine.isHISE() && Engine.isPlugin() == false)
{
	// Set Default Audio Device
	for (x in Settings.getAvailableDeviceNames())
	{
		if (x.contains("Babyface Pro") && Settings.getCurrentAudioDevice() != x)
		{
			Settings.setAudioDevice(x);
			Settings.setBufferSize(256);
			break;
		}
	}
	
	// Set Default MIDI Device
	for (x in Settings.getMidiInputDevices())
	{
		if (x == "SL STUDIO Port 1") Settings.toggleMidiInput(x, true);
		if (x == "SL STUDIO Port 2") Settings.toggleMidiInput(x, false);
	}
}
```

### [40] Broadcaster Module Parameter Watcher
URL: https://forum.hise.audio/topic/8741
Tags: forum, Scripting, author
Mechanical tier: 1
Description: Engine.createBroadcaster with attachToModuleParameter to listen for changes to an Audio Loop Player's RootNote parameter. addListener callback receives the new value whenever the module attribute changes.
```javascript
const var bc = Engine.createBroadcaster({
	"id": "root watcher",
	"args": ["processor", "parameter", "value"]
});

bc.attachToModuleParameter("Audio Loop Player1", "RootNote", "watch attribute");

bc.addListener("", "update something", function(u1, u2, value)
{
	Console.print(value);
});
```

### [41] Toggle Knob Mode via Button
URL: https://forum.hise.audio/topic/9250
Tags: forum, Scripting, expert
Mechanical tier: 2
Description: setMode on a knob to switch between TempoSync and Frequency modes from a button callback. Demonstrates runtime mode switching using the ternary operator with setControlCallback.
```javascript
const var Knob1 = Content.getComponent("Knob1");

inline function onButton1Control(component, value)
{
	Knob1.setMode(value == 0 ? "TempoSync" : "Frequency");
};

Content.getComponent("Button1").setControlCallback(onButton1Control);
```

### [42] Keyboard LAF Black and White Notes
URL: https://forum.hise.audio/topic/9394
Tags: forum, Scripting, expert
Mechanical tier: 2
Description: registerFunction drawBlackNote and drawWhiteNote to customise a keyboard FloatingTile's appearance. Sets per-note fill colours and adjusts area dimensions for custom keyboard rendering via setLocalLookAndFeel.
```javascript
const laf = Content.createLocalLookAndFeel();

laf.registerFunction("drawBlackNote", function(g, obj)
{	
	var a = obj.area;

	g.setColour(Colours.black);
	g.fillRect([a[0], a[1], a[2] + 4, a[3]]);
	
});

laf.registerFunction("drawWhiteNote", function(g, obj)
{
	var a = obj.area;

	g.setColour(Colours.white);
	g.fillRect([a[0], a[1], a[2], a[3]]);
});

const var FloatingTile1 = Content.getComponent("FloatingTile1");
FloatingTile1.setLocalLookAndFeel(laf);
```

### [43] Error Handler for Missing Samples
URL: https://forum.hise.audio/topic/8971
Tags: forum, Scripting, trusted
Mechanical tier: 1
Description: Engine.createErrorHandler with setErrorCallback to detect missing samples at runtime. Checks state code 9 to trigger a custom overlay, with getNumActiveErrors and getCurrentErrorLevel for detailed error reporting.
```javascript
const var errorHandler = Engine.createErrorHandler();
	
	errorHandler.setErrorCallback(function(state, message)
	{
		 ErrorHandler.errorLabel.set("text", errorHandler.getNumActiveErrors() + " " + errorHandler.getCurrentErrorLevel() + " " + state + " " + message);
		 
		if (state == 9) // if samples not detected
		{
			// show your own overlay here
		}
		 
	});
```

### [44] Browse and Set Sample Folder
URL: https://forum.hise.audio/topic/8971
Tags: forum, Scripting, trusted
Mechanical tier: 2
Description: FileSystem.browseForDirectory with Settings.setSampleFolder and Engine.reloadAllSamples to let users relocate missing samples at runtime. Includes an init-guard pattern to prevent the callback from firing on component initialization.
```javascript
inline function onlocateSamplesButtonControl(component, value)
	{
		if (canExecute.locateButton) // prevent execution on init
		{
			FileSystem.browseForDirectory(FileSystem.UserHome, function(result)
			{
				Settings.setSampleFolder(result);
				// remove overlay
				Engine.reloadAllSamples();
				
			});
		}
		else canExecute.locateButton = true;
	};
	
	Content.getComponent("locateSamplesButton").setControlCallback(onlocateSamplesButtonControl);
```

### [45] Custom Table Path LAF
URL: https://forum.hise.audio/topic/8333
Tags: forum, Scripting, trusted
Mechanical tier: 1
Description: registerFunction drawTablePath with path string parsing to customize the table curve rendering. Manipulates the SVG path string via split/join and fromString to modify the default table path shape before drawing.
```javascript
TBLLaf.registerFunction("drawTablePath", function(g, obj)
{
	var p = Content.createPath();
	p.clear();
	var str = obj.path.toString();
	var arr = [];
	
	if (str.contains("q"))
	{
		str = str.substring(0, str.lastIndexOf("l"));
		var arr = str.split(" ");
	}
	
	else
	{
		arr = str.split(" ");
		for (i = 0; i < 3; i++)
			arr.pop();
	}
	
	arr[10] = arr[13];
	
	str = arr.join(" ");
	
	p.fromString(str);
	g.setColour(Colours.withAlpha(Colours.cadetblue, 1));
	g.drawPath(p, p.getBounds(1), 1.5);
});
```

### [46] Custom MIDI Event Data Storage
URL: https://forum.hise.audio/topic/8610
Tags: forum, Scripting, author
Mechanical tier: 1
Description: Global 2D array indexed by Message.getEventId modulo slot count to attach custom data to MIDI events. Provides setCustomValue and getCustomValue accessors that survive across MIDI processor boundaries using a shared global namespace.
```javascript
namespace CustomMessage
{
const var NUM_SLOTS = 512;
const var NUM_PER_SLOT = 16;

if(!isDefined(GLOBAL_STORAGE))
{
 global GLOBAL_STORAGE = [];
 GLOBAL_STORAGE.reserve(512);

 for(i = 0; i < NUM_SLOTS; i++)
 {
 GLOBAL_STORAGE[i] = [];
 for(j = 0; j < NUM_PER_SLOT; j++)
 GLOBAL_STORAGE[i][j] = 0;
 }
}

inline function setCustomValue(index, value)
{
 GLOBAL_STORAGE[Message.getEventId() % NUM_SLOTS][index] = value;
}

inline function getCustomValue(index)
{
 return GLOBAL_STORAGE[Message.getEventId() % NUM_SLOTS][index];
}
}
```

### [47] Modal Text Input with Custom Properties
URL: https://forum.hise.audio/topic/8068
Tags: forum, Scripting, author
Mechanical tier: 2
Description: Content.showModalTextInput with a properties object to configure font, position, colours, and initial text for a popup text field. Callback receives ok/cancel state and the entered text for validation.
```javascript
const var prop =
{
	"parentComponent": "Panel1", // use an empty string for global coordinates
	"fontName": "Comic Sans MS", 
	"x": 10,					 // these positions are relative to the parentComponent
	"y": 10,					 // if you omit these and the area is empty, it will use
	"width": 90,				 // the default positioning from the slider text box
	"height": 24,
	"fontSize": 18,
	"alignment": "right",
	"fontStyle": "Bold",
	"bgColour": 0xFF00FF00,
	"itemColour": 16679297,
	"textColour": 4287455585,
	"text": "funkyboy"			// the initial text to display
};

// the minimum properties will show the default slider popup
const var minProp = 
{
	"text": "some text"
};

inline function onButton1Control(component, value)
{
	Content.showModalTextInput(prop, function(ok, input)
	{
		Console.print(ok);
		Console.print(input);
	});
};

Content.getComponent("Button1").setControlCallback(onButton1Control);
```

### [48] Shift-Click Text Input for Knobs
URL: https://forum.hise.audio/topic/8068
Tags: forum, Scripting, author
Mechanical tier: 2
Description: Engine.createBroadcaster with attachToComponentMouseEvents to show Content.showModalTextInput on shift-click for multiple knobs. Calculates popup position from getGlobalPositionX/Y and formats the initial value using Engine.doubleToString with step-size precision.
```javascript
const var textInputBroadcaster = Engine.createBroadcaster({
	"id": "text-input",
	"args": ["component", "value"],
	"tags": ["text-input"]
});

const var List = [Content.getComponent("Knob5"),
 Content.getComponent("Knob4"),
 Content.getComponent("Knob3"),
 Content.getComponent("Knob2"),
 Content.getComponent("Knob1")];

textInputBroadcaster.attachToComponentMouseEvents(List, "Clicks Only", "");

textInputBroadcaster.addListener("", "show textbox", function(component, event)
{
	if(event.shiftDown && !event.mouseUp)
	{
		var obj = {
			"text": Engine.doubleToString(component.getValue(), parseInt(Math.log10(component.get("stepSize")) * -1)) + " " + component.get("suffix"),
			"x": component.getGlobalPositionX() + component.get("width") / 2 - 30,
			"y": component.getGlobalPositionY() + component.get("height") / 2 - 12,
			"width": 60,
			"height": 24
		};
		
		var c = component;
		
		Content.showModalTextInput(obj, function[c](ok, input)
		{
			if(ok)
			{
				c.setValue(parseFloat(input));
				c.changed();
			}
		});
	}
});
```

### [49] Save Expansion User Preset with Path Validation
URL: https://forum.hise.audio/topic/5913
Tags: forum, Scripting, expert
Mechanical tier: 1
Description: FileSystem.browse with Expansions.getCurrentUserPresetsFolder to save user presets within the correct expansion hierarchy. Validates the chosen path is nested at least three directories deep using getParentDirectory chain before calling Engine.saveUserPreset.
```javascript
inline function createNewPreset()
 {
		local userPresetsFolder = Expansions.getCurrentUserPresetsFolder();

		if (!isDefined(userPresetsFolder))
			return;

	 FileSystem.browse(userPresetsFolder, true, "*.preset", function[userPresetsFolder](f)
	 {
			var grandparent = f.getParentDirectory().getParentDirectory().getParentDirectory();

			if (!userPresetsFolder.isSameFileAs(grandparent))
				return Engine.showMessageBox("Invalid Location", "Presets must be saved in a bank and category folder within the instrument's user presets folder.", 0);
			Engine.saveUserPreset(f.toString(f.FullPath).replace(".preset"));
	 });
 }
```

### [50] Get Expansion Names List
URL: https://forum.hise.audio/topic/7772
Tags: forum, Scripting, expert
Mechanical tier: 1
Description: Engine.createExpansionHandler with getExpansionList and getProperties to extract expansion names into an array. Iterates installed expansions and collects their Name property for UI display or logic.
```javascript
const var expHandler = Engine.createExpansionHandler();
const var expList = expHandler.getExpansionList();
const var expNames = [];

for (e in expList)
	expNames.push(e.getProperties().Name);
```

### [51] MIDI Player Sync to Master Clock
URL: https://forum.hise.audio/topic/7396
Tags: forum, Scripting, trusted, solved
Mechanical tier: 3
Description: Synth.getMidiPlayer with setUseTimestampInTicks and setSyncToMasterClock to configure multiple MIDI players for synchronized tick-based playback. Iterates an array of player references to apply shared timing settings.
```javascript
const var yourMidiPlayers = [Synth.getMidiPlayer("myMidiPlayer1"),
			 Synth.getMidiPlayer("myMidiPlayer2"),
			 Synth.getMidiPlayer("myMidiPlayer3")];

for (p in yourMidiPlayers)
{
	p.setUseTimestampInTicks();
	p.setSyncToMasterClock();
}
```

### [52] Animated Disclosure Triangle Panel
URL: https://forum.hise.audio/topic/7335
Tags: forum, Scripting, trusted
Mechanical tier: 2
Description: Panel with setPaintRoutine using Graphics.rotate for an animated triangle, setMouseCallback for toggle, and setTimerCallback for smooth 90-degree rotation animation. Uses panel data object for state and angle tracking with timer-driven repaint.
```javascript
const var panel = Content.getComponent("panel");
panel.data.angle = 0;
panel.data.state = false;

// Paint Routine
panel.setPaintRoutine(function(g)
{
	g.setColour(Colours.whitesmoke);
	
	g.rotate(Math.toRadians(this.data.angle), [this.getWidth()/2, this.getHeight()/2]);
	
	// triangle reduced by 30% to avoid edge collision (sqrt((w*w)+(h*h)) = 0.707)
	var area = [this.getWidth()*0.15, this.getWidth()*0.15, this.getWidth()*0.7, this.getHeight()*0.7];
	
	g.fillTriangle(area, Math.PI/2);
});

// Mouse CB
panel.setMouseCallback(function(event)
{
	if (event.clicked)
	{
		this.data.state = !this.data.state;
		this.startTimer(30);
	}
});

// Timer
panel.setTimerCallback(function()
{
	if (this.data.state)
	{
		if (panel.data.angle < 90)
			panel.data.angle += 10;
		else
			this.stopTimer();
	}
	else
	{	
		if (panel.data.angle > 0)
			panel.data.angle -= 10;
		else
			this.stopTimer();
	}
	
	this.repaint();
});
```

### [53] Broadcaster Listener with Component Binding
URL: https://forum.hise.audio/topic/6467
Tags: forum, Scripting, author
Mechanical tier: 2
Description: Engine.createBroadcaster with addListener binding 'this' to a UI component for state-driven updates. The listener callback uses the component passed as first argument to addListener, enabling direct property changes via this.set.
```javascript
const var btnLogout = Content.getComponent("btnLogout");

const var isDownloading = Engine.createBroadcaster({
	"id": "Download Status",
	"args": ["state"]
});

isDownloading.addListener(btnLogout, "Disable the Logout Button during download", function(state)
{
	this.set("enabled", !state);
});
```

### [54] Serialize Component Values with SliderPack Support
URL: https://forum.hise.audio/topic/5818
Tags: forum, Scripting, expert
Mechanical tier: 1
Description: Content.getAllComponents with type check for ScriptSliderPack to correctly serialize multi-value components. Uses getNumSliders and getSliderValueAt to capture individual slider values as arrays alongside standard component values.
```javascript
const var myObj = {};

for (c in Content.getAllComponents(""))
{
	if (c.get("type") == "ScriptSliderPack")
	{
		var values = [];

		for (j = 0; j < c.getNumSliders(); j++)
			values.push(c.getSliderValueAt(j));

		myObj[c.get("id")] = values;
	}
	else
	{
		myObj[c.get("id")] = c.getValue();
	}
}
```

### [55] Filter Effects Array by ID Pattern
URL: https://forum.hise.audio/topic/4690
Tags: forum, Scripting, expert
Mechanical tier: 1
Description: Synth.getAllEffects with indexOf check to remove effects whose IDs contain an underscore. Demonstrates runtime effect list filtering by naming convention for selective processing.
```javascript
const fx = Synth.getAllEffects("");

for (i = 0; i < fx.length; i++)
 if (fx[i].getId().indexOf("_") != -1) fx.remove(fx[i]);
```

### [56] Decimal to Hex String Converter
URL: https://forum.hise.audio/topic/5678
Tags: forum, Scripting
Mechanical tier: 1
Description: Bitwise right-shift and AND operations to convert a decimal ARGB colour value to a hex string. Useful for colour manipulation and debugging when working with HISE's integer-based colour representation.
```javascript
inline function toHex(num, digits) 
{
	local hexTable = ["0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "A", "B", "C", "D", "E", "F"];
	local n = num;
	local s = digits;
	local result = "";
	while (s--)
	{
		result = hexTable[n & 0xF] + result;
		n = n >> 4;
	}
	return result;
}

Console.print(toHex(4278221567, 8)); 
// output: FF007AFF
```

### [57] Remove All Child Panels
URL: https://forum.hise.audio/topic/4652
Tags: forum, Scripting, expert
Mechanical tier: 1
Description: getChildPanelList and removeFromParent to clean up dynamically created child panels. Call during onInit to prevent stale child panels from accumulating across script recompiles.
```javascript
inline function removeAllChildren(panel)
{
 local children = panel.getChildPanelList();

 if (children.length > 0)
	for (x in children) x.removeFromParent();
}
```

### [58] Custom Array Sort Function
URL: https://forum.hise.audio/topic/5147
Tags: forum, Scripting, expert
Mechanical tier: 1
Description: Engine.sortWithFunction with a comparator callback for numeric array sorting. The comparator returns -1 for less-than and boolean coercion for greater-than, following the standard comparison function pattern.
```javascript
reg arr = [5, 3, 6, 4, 1, 2];

Engine.sortWithFunction(arr, function(a, b)
{
 if (a < b) return -1 else return a > b;
});

Console.print(trace(arr));
```

### [59] Animated Circle Radar with Blur Effects
URL: https://forum.hise.audio/topic/3507
Tags: forum, Scripting, author
Mechanical tier: 2
Description: Panel setPaintRoutine using beginLayer, gaussianBlur, addNoise, and applyMask for a sci-fi radar effect. Draws concentric ellipses scaled by panel value, applies post-processing blur and noise, then masks to a circular path.
```javascript
Content.makeFrontInterface(150, 150);

const var Panel1 = Content.getComponent("Panel1");
Panel1.setValue(1);

const var circlePath = Content.createPath();

circlePath.addArc([0.0, 0.0, 1.0, 1.0], 0.0, Math.PI * 2);

Panel1.setPaintRoutine(function(g)
{
 g.beginLayer(true);
 g.setColour(Colours.white);
 
 var area = [1, 1, this.getWidth() - 2, this.getHeight() - 2];
 
 g.drawEllipse(area, 2);
 
 var radius = this.getWidth() / 2 * this.getValue();
 var diameter;
 var pos;

 for (i = 1; i < 4; i++)
 {
 diameter = (radius * 2 / 4 * i);
 pos = this.getWidth() / 2 - diameter / 2;
 if (pos > 0)
 g.drawEllipse([pos + 1, pos + 1, diameter - 2, diameter - 2], 2);
 } 
 
 g.gaussianBlur(1.0 / this.getValue() * 12.0);
 g.addNoise(0.2);
 g.applyMask(circlePath, area, false);
 g.endLayer();
});

inline function onKnob1Control(component, value)
{
	Panel1.setValue(component.get("max") + 1 - value);
	Panel1.repaint();
};

Content.getComponent("Knob1").setControlCallback(onKnob1Control);
```

### [60] Colour Saturation Gradient Paint Routine
URL: https://forum.hise.audio/topic/4009
Tags: forum, Scripting, author
Mechanical tier: 2
Description: Bitwise colour manipulation in a ColourOps namespace to multiply saturation of an ARGB integer. Combined with a gradient fill loop in setPaintRoutine to create a horizontal desaturation fade effect.
```javascript
const var Panel1 = Content.getComponent("Panel1");

namespace ColourOps
{
	inline function withMultipliedSaturation(colour, saturation)
	{
		local a = (colour & 0xFF000000);
		local r = (colour & 0x00FF0000) >> 16;
		local g = (colour & 0x0000FF00) >> 8;
		local b = (colour & 0x000000FF);

		local max = Math.max(r, Math.max(g, b));

		local rDiff = max - r;
		local gDiff = max - g;
		local bDiff = max - b;

		local invSat = 1.0 - saturation;

		r = parseInt(r + invSat * rDiff);
		g = parseInt(g + invSat * gDiff);
		b = parseInt(b + invSat * bDiff);

		return a | (r << 16) | (g << 8) | b;
	}
}

Panel1.setPaintRoutine(function(g)
{
	var c = this.get("itemColour");
	var area = [0, 0, 6.0, this.getHeight()];
	var gradientData = [undefined, undefined, 0.0, Colours.black, undefined, this.getHeight()];

	for (var x = 0; x < this.getWidth(); x += 4)
	{
		gradientData[0] = ColourOps.withMultipliedSaturation(c, x / this.getWidth());
		gradientData[1] = x;
		gradientData[4] = x;

		area[0] = x;

		g.setGradientFill(gradientData);
		g.fillRect(area);
	}
});
```

### [61] Sequential Server File Download
URL: https://forum.hise.audio/topic/3849
Tags: forum, Scripting, expert
Mechanical tier: 4
Description: Server.downloadFile with recursive downloadCallback to chain sequential downloads. Each completed download triggers the next via startNextDownload, avoiding concurrent download limits by processing the image list one at a time.
```javascript
Content.makeFrontInterface(600, 500);

Console.clear();

Server.setBaseURL("

const images = ["12-profileimg.jpg", "67-profileavatar.jpeg", "121-profileimg.jpg", "338-profileavatar.png"];

function downloadCallback()
{
 if (this.data.finished)
 { 
 if (count < images.length-1)
 {
 count++;
 startNextDownload(images[count]);
 }
 }
}

inline function startNextDownload(fileName)
{
 Console.print(fileName);
 local f = FileSystem.getFolder(FileSystem.Downloads).getChildFile(fileName);
 Server.downloadFile(fileName, {}, f, downloadCallback);
}

reg count = 0;
startNextDownload(images[0]);
```

### [62] Deep Nested User Preset Save
URL: https://forum.hise.audio/topic/844
Tags: forum, Scripting, author
Mechanical tier: 1
Description: FileSystem.getFile with getChildFile using a deeply nested path string to save user presets in custom subfolder hierarchies. Engine.saveUserPreset accepts a File object for arbitrary preset storage locations.
```javascript
const var f = FileSystem.getFile(FileSystem.UserPresets).getChildFile("MyFunkySubFolder/AnotherSubFolder/3LevelSubfolder/Mindblowing4LevelSubfolder/Preset.preset");

Engine.saveUserPreset(f);
```

### [63] Browse and Load or Save User Preset
URL: https://forum.hise.audio/topic/844
Tags: forum, Scripting, author
Mechanical tier: 1
Description: FileSystem.browse with Engine.loadUserPreset and Engine.saveUserPreset for a minimal custom preset browser. Uses a boolean flag to switch between load (false) and save (true) browse modes with .preset file filtering.
```javascript
function load(file)
{
 Engine.loadUserPreset(file);
}

function save(file)
{
 Engine.saveUserPreset(file);
}

// Place this anyway you like (button callbacks, whatever)...
var shouldLoad = true;

if(shouldLoad)
 FileSystem.browse(FileSystem.UserPresets, false, "*.preset", load);
else
 FileSystem.browse(FileSystem.UserPresets, true, "*.preset", save);
```

### [64] Save MIDI Player to File
URL: https://forum.hise.audio/topic/3273
Tags: forum, Scripting, author
Mechanical tier: 1
Description: FileSystem.browse with MIDIPlayer.saveAsMidiFile to export MIDI data to a user-chosen .mid file. Combines file dialog with the MIDI player's save method for user-initiated MIDI export.
```javascript
FileSystem.browse(FileSystem.Desktop, true, "*.mid", function(f)
{
 MIDIPlayer1.saveAsMidiFile(f.toString(0), 1);
});
```

### [65] A/B Preset Comparison Toggle
URL: https://forum.hise.audio/topic/1197
Tags: forum, Scripting, trusted
Mechanical tier: 2
Description: Button callback swaps between two stored states (A and B) of all UI components. Captures current values before restoring the alternate set, with changed() calls to propagate updates through the signal chain.
```javascript
Content.makeFrontInterface(600, 400);

const var Compare_Elements_Array = Content.getAllComponents("");
const var Compare_A = [];
const var Compare_B = [];

for (i = 0; i < Compare_Elements_Array.length; i++)
{
	Compare_A[i] = 0;
	Compare_B[i] = 0;
}

inline function onCompareControl(component, value)
{
	if (!value)
	{
		for (i = 0; i < Compare_Elements_Array.length; i++)
		{
			Compare_A[i] = Compare_Elements_Array[i].getValue();
			Compare_Elements_Array[i].setValue(Compare_B[i]);
		}
	}
	else
	{
		for (i = 0; i < Compare_Elements_Array.length; i++)
		{
			Compare_B[i] = Compare_Elements_Array[i].getValue();
			Compare_Elements_Array[i].setValue(Compare_A[i]);
		}
	}

	for (comp in Compare_Elements_Array)
		comp.changed();
}

Content.getComponent("Compare").setControlCallback(onCompareControl);
```

### [66] Persistent Data via Hidden Panel
URL: https://forum.hise.audio/topic/52
Tags: forum, Scripting, expert
Mechanical tier: 1
Description: Hidden ScriptPanel with saveInPreset and a control callback to persist arbitrary key-value data across preset loads. PersistentData namespace provides get/set accessors that store data in the panel's value, leveraging HISE's built-in preset serialization.
```javascript
namespace PersistentData
{
 //The data object. This will be overwritten by the onControl callback of the restorer
 reg _persistentData = {};
 
 /*This panel will restore the storage object in its control callback. 
 If the value is not an object, it won't do anything but use the given
 storage variable as value*/
 const var pnlDataRestorer = Content.addPanel("pnlDataRestorer", 0, 0);
 pnlDataRestorer.set("saveInPreset", true);
 pnlDataRestorer.set("visible", false);
 
 pnlDataRestorer.setControlCallback(onpnlDataRestorerControl); 
 inline function onpnlDataRestorerControl(component, value)
 {
 // Check if the panel's value is an object
 // If not, set the value to the storage object
 
 if (typeof(value) == "object")
 _persistentData = value; //Copy the object from the panel to the storage object
 else 
 component.setValue(_persistentData); //Initialize the restorer
 };
 
 inline function set(id, value)
 {
 _persistentData[id] = value;
 pnlDataRestorer.setValue(_persistentData);
 }
 
 inline function get(id)
 {
 return _persistentData[id];
 }
}
```

### [67] Local Iterator Variable in Inline Functions
URL: https://forum.hise.audio/topic/2157
Tags: forum, Scripting, author
Mechanical tier: 1
Description: Declaring local i before a for loop inside inline functions to prevent shared global iterator conflicts. Without the local declaration, nested inline function calls share the same loop counter causing infinite loops.
```javascript
inline function one()
{
 local i = 0;
 for (i = 0; i < 20; i++)
 {
 two();
 }
}

inline function two()
{
 local i = 0;
 for (i = 0; i < 10; i++)
 {
 Console.print(i);
 }
}
```

### [68] Panel Timer Countdown with Label
URL: https://forum.hise.audio/topic/1489
Tags: forum, Scripting, expert
Mechanical tier: 2
Description: Panel setTimerCallback for a counting animation that updates a Label and hides the panel at completion. Uses panel value as counter, startTimer/stopTimer for timed execution, and showControl for visibility toggling.
```javascript
Content.makeFrontInterface(600, 500);

const var Label1 = Content.getComponent("Label1");
Label1.set("text", "0");

const var Panel1 = Content.getComponent("Panel1");
Panel1.setValue(0);

Panel1.setTimerCallback(function()
{
	this.setValue(this.getValue() + 1);
	Label1.set("text", this.getValue());

	if (this.getValue() == 10)
	{
		this.showControl(false);
		this.stopTimer();
	}
});

inline function onbtnStartControl(component, value)
{
	local rate = 500;
	Label1.set("text", "0");
	Panel1.showControl(true);
	Panel1.setValue(0);
	Panel1.startTimer(rate);
}

Content.getComponent("btnStart").setControlCallback(onbtnStartControl);
```

### [69] Set Keyboard Key Colour
URL: https://forum.hise.audio/topic/1043
Tags: forum, Scripting, expert
Mechanical tier: 1
Description: Engine.setKeyColour with an ARGB hex value to highlight a specific MIDI note on the on-screen keyboard. Sets note 60 (middle C) to a semi-transparent cyan colour.
```javascript
Content.makeFrontInterface(600, 500);

const var key_low = 0x6000ffff;
Engine.setKeyColour(60, key_low);
```

### [70] Interactive ARGB Colour Picker
URL: https://forum.hise.audio/topic/395
Tags: forum, Scripting, expert
Mechanical tier: 1
Description: Four Content.addKnob sliders for ARGB channels with a convertToHex utility and live panel preview. Updates a colour string and repaints a preview panel on each knob change, displaying the hex colour code in a label.
```javascript
/**
 * Title: colourPicker.js
 * Author: David Healey
 * Date: 07/09/2017
 * Modified: 07/09/2017
 * License: Public Domain
*/

Content.setHeight(175);

reg colour = [];
reg colourString;

const var alpha = Content.addKnob("Alpha", 0, 0);
alpha.setRange(0, 255, 1);
alpha.set("style", "Vertical");
alpha.set("itemColour", 0xFFAAAAAA);
alpha.set("bgColour", 0xFF000000);
const var red = Content.addKnob("Red", 150, 0);
red.setRange(0, 255, 1);
red.set("style", "Vertical");
red.set("itemColour", 0xFFFF0000);
red.set("bgColour", 0xFF000000);
const var green = Content.addKnob("Green", 300, 0);
green.setRange(0, 255, 1);
green.set("itemColour", 0xFF64FF4E);
green.set("bgColour", 0xFF000000);
green.set("style", "Vertical");
const var blue = Content.addKnob("Blue", 450, 0);
blue.setRange(0, 255, 1);
blue.set("style", "Vertical");
blue.set("itemColour", 0xFF1C00FF);
blue.set("bgColour", 0xFF000000);

const var code = Content.addLabel("code", 600, 10);
code.set("bgColour", 0xFF000000);

const var pnlColour = Content.addPanel("pnlColour", 0, 50);
pnlColour.set("width", 750);
pnlColour.set("height", 100);
pnlColour.setPaintRoutine(function(g){g.fillAll(parseInt(colourString));});

inline function convertToHex(v)
{
	reg hexTable = ["0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "A", "B", "C", "D", "E", "F"];
	reg d1 = hexTable[Math.floor(v/16)];
	reg d2 = hexTable[Math.floor(v % 16)];
	
	return d1 + d2;
}

inline function updateCode()
{
	code.set("text", "0x" + colour[0] + colour[1] + colour[2] + colour[3]);
	colourString = "0x" + colour[0] + colour[1] + colour[2] + colour[3];	
}function onNoteOn()
{
	
}
function onNoteOff()
{
	
}
function onController()
{
	
}
function onTimer()
{
	
}
function onControl(number, value)
{
	switch (number)
	{
		case alpha:
			colour[0] = convertToHex(parseInt(value));
			updateCode();
			pnlColour.repaintImmediately();
		break;
		
		case red:
			colour[1] = convertToHex(parseInt(value));
			updateCode();
			pnlColour.repaintImmediately();
		break;
		
		case green:
			colour[2] = convertToHex(parseInt(value));
			updateCode();
			pnlColour.repaintImmediately();
		break;
		
		case blue:
			colour[3] = convertToHex(parseInt(value));
			updateCode();
			pnlColour.repaintImmediately();
		break;		
	}
}
```

### [71] Basic Graphics Drawing Primitives
URL: https://forum.hise.audio/topic/169
Tags: forum, Scripting, expert
Mechanical tier: 1
Description: Panel setPaintRoutine with drawEllipse, drawTriangle, and drawLine to render basic shapes. Shows the area array format [x, y, width, height] and border-size parameter for each Graphics drawing method.
```javascript
Content.setHeight(150);

var area = Content.addPanel("area", 0, 0);

// [/JSON area]
area.set("width", 150);
area.set("height", 150);

// Define a paint routine
area.setPaintRoutine(function(g)
{
	g.setColour(4294901730);
	g.drawEllipse([10, 10, 100, 100], 5); //x, y, width, height, border size
	g.drawEllipse([30, 40, 20, 20], 5); //x, y, width, height, border size	
	g.drawEllipse([70, 40, 20, 20], 5); //x, y, width, height, border size	
	g.drawTriangle([55, 60, 10, 10], 0, 3);
	g.drawLine(45, 75, 90, 90, 5);
});
```

### [72] Knob Configuration with setPropertiesFromJSON
URL: https://forum.hise.audio/topic/380
Tags: forum, Scripting, author
Mechanical tier: 1
Description: Content.setPropertiesFromJSON to configure a knob's min, max, mode, stepSize, middlePosition, and suffix in a single call. Shows the JSON property object format for batch-setting component properties including Decibel mode.
```javascript
Content.makeFrontInterface(600, 500);
const var Knob = Content.addKnob("Knob", 76, 19);
// [JSON Knob]
Content.setPropertiesFromJSON("Knob", {
 "min": -100,
 "max": 0,
 "saveInPreset": 0,
 "mode": "Decibel",
 "stepSize": 0.1,
 "middlePosition": -12,
 "suffix": " dB"
});
// [/JSON Knob]

Knob.set("saveInPreset", false);;
Knob.setValue(-18.0);
Console.print(Knob.getValueNormalized()); // 0.34...
```
