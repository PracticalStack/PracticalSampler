namespace DRSContent
{
const var mainSamplerId = "DRS Main Sampler";
const var outputGainId = "DRS Output Gain";
const var spaceEffectId = "DRS Space";
const var envelopeId = "DRS Amp Envelope";
const var velocityId = "DRS Velocity";

const var sampleMaps = ["DRS_SinePad", "DRS_TriangleLead"];
const var factoryPresets = ["DRS Init", "DRS Triangle Motion"];

inline function toMenuItems(list)
{
	return list.join("\n");
}

inline function describeSelection(sampleMapIndex, presetIndex, tone, motion, space)
{
	local sampleMapName = sampleMaps[sampleMapIndex];
	local presetName = factoryPresets[presetIndex];

	return "Map: " + sampleMapName
		+ " | Preset: " + presetName
		+ " | Tone: " + Engine.doubleToString(tone, 2)
		+ " | Motion: " + Engine.doubleToString(motion, 2)
		+ " | Space: " + Engine.doubleToString(space, 2)
		+ " | Sampler: " + mainSamplerId
		+ " | Gain: " + outputGainId;
}
}
