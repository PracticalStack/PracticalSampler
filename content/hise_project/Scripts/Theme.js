namespace DRSTheme
{
const var backgroundColour = 0xFF121723;
const var panelColour = 0xFF1D2536;
const var accentColour = 0xFFE0B04B;
const var textColour = 0xFFF4F1E8;
const var mutedTextColour = 0xFF97A3BA;

inline function styleKnob(name, text, x, y)
{
	local knob = Content.addKnob(name, x, y);

	Content.setPropertiesFromJSON(name, {
		"text": text,
		"width": 128,
		"height": 128,
		"min": 0.0,
		"max": 1.0,
		"middlePosition": 0.5,
		"stepSize": 0.01,
		"suffix": ""
	});

	return knob;
}

inline function styleLabel(name, text, x, y, width, height)
{
	local label = Content.addLabel(name, x, y);

	Content.setPropertiesFromJSON(name, {
		"text": text,
		"width": width,
		"height": height,
		"fontName": "Oxygen",
		"fontSize": 16.0,
		"textColour": textColour,
		"bgColour": 0x00000000,
		"alignment": "left"
	});

	return label;
}
}
