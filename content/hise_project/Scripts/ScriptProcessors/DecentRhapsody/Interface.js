include("Theme.js");
include("Data/ContentModel.js");

Content.makeFrontInterface(920, 540);

const var DRSMainSampler = Synth.getSampler(DRSContent.mainSamplerId);
const var DRSOutputGain = Synth.getEffect(DRSContent.outputGainId);

const var BackgroundPanel = Content.addPanel("BackgroundPanel", 0, 0);
Content.setPropertiesFromJSON("BackgroundPanel", {
	"width": 920,
	"height": 540
});

BackgroundPanel.setPaintRoutine(function(g)
{
	g.fillAll(DRSTheme.backgroundColour);
});

const var Header = DRSTheme.styleLabel("Header", "Decent Rhapsody Studio", 36, 28, 360, 32);
Header.set("fontSize", 28.0);

const var Subheader = DRSTheme.styleLabel("Subheader", "Phase 0 frontend shell", 36, 64, 360, 24);
Subheader.set("textColour", DRSTheme.mutedTextColour);

const var StatusLabel = DRSTheme.styleLabel("StatusLabel", "Awaiting selection", 36, 432, 848, 28);
StatusLabel.set("bgColour", DRSTheme.panelColour);

const var FooterLabel = DRSTheme.styleLabel("FooterLabel", "Sample maps and factory presets are coming from content/hise_project.", 36, 470, 848, 24);
FooterLabel.set("textColour", DRSTheme.mutedTextColour);

const var SampleMapSelector = Content.addComboBox("SampleMapSelector", 36, 126);
Content.setPropertiesFromJSON("SampleMapSelector", {
	"width": 260,
	"height": 40,
	"items": "",
	"text": "Sample Map"
});

const var FactoryPresetSelector = Content.addComboBox("FactoryPresetSelector", 328, 126);
Content.setPropertiesFromJSON("FactoryPresetSelector", {
	"width": 260,
	"height": 40,
	"items": DRSContent.toMenuItems(DRSContent.factoryPresets),
	"text": "Factory Preset"
});

const var ToneKnob = DRSTheme.styleKnob("ToneKnob", "Tone", 36, 212);
const var MotionKnob = DRSTheme.styleKnob("MotionKnob", "Motion", 196, 212);
const var SpaceKnob = DRSTheme.styleKnob("SpaceKnob", "Space", 356, 212);

const var HeroPanel = Content.addPanel("HeroPanel", 612, 126);
Content.setPropertiesFromJSON("HeroPanel", {
	"width": 272,
	"height": 248,
	"bgColour": DRSTheme.panelColour,
	"itemColour": DRSTheme.accentColour,
	"textColour": DRSTheme.textColour
});

HeroPanel.setPaintRoutine(function(g)
{
	local a = this.getLocalBounds(0);

	g.fillAll(DRSTheme.panelColour);
	g.setColour(0x22FFFFFF);
	g.drawRoundedRectangle([0, 0, a[2], a[3]], 18.0, 1.0);

	g.setColour(DRSTheme.accentColour);
	g.fillRoundedRectangle([20, 24, 72, 8], 4.0);

	g.setColour(DRSTheme.textColour);
	g.setFont("Oxygen Bold", 24.0);
	g.drawAlignedText("Frontend\nAuthoring Shell", [20, 52, a[2] - 40, 80], "left");

	g.setColour(DRSTheme.mutedTextColour);
	g.setFont("Oxygen", 15.0);
	g.drawAlignedText("Grounded in first-party presets,\nsample maps, and project metadata.", [20, 146, a[2] - 40, 64], "left");
});

inline function updateStatus()
{
	local sampleMapIndex = Math.max(0, SampleMapSelector.getValue() - 1);
	local presetIndex = Math.max(0, FactoryPresetSelector.getValue() - 1);

	StatusLabel.set("text", DRSContent.describeSelection(
		sampleMapIndex,
		presetIndex,
		ToneKnob.getValue(),
		MotionKnob.getValue(),
		SpaceKnob.getValue()
	));
}

inline function onSelectionControl(component, value)
{
	if(component == SampleMapSelector && value)
		DRSMainSampler.loadSampleMap(SampleMapSelector.getItemText());

	if(component == ToneKnob)
		DRSOutputGain.setAttribute(DRSOutputGain.Gain, -18.0 + (24.0 * value));

	updateStatus();
}

SampleMapSelector.setControlCallback(onSelectionControl);
FactoryPresetSelector.setControlCallback(onSelectionControl);
ToneKnob.setControlCallback(onSelectionControl);
MotionKnob.setControlCallback(onSelectionControl);
SpaceKnob.setControlCallback(onSelectionControl);

const var sampleMapList = Sampler.getSampleMapList();

for (sampleMap in sampleMapList)
	SampleMapSelector.addItem(sampleMap);

SampleMapSelector.setValue(1);
FactoryPresetSelector.setValue(1);
ToneKnob.setValue(0.50);
MotionKnob.setValue(0.25);
SpaceKnob.setValue(0.18);

updateStatus();

function onNoteOn()
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
}
