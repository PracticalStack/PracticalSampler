SliderPackProcessor (object)
Obtain via: Synth.getSliderPackProcessor(processorId)

Lightweight wrapper around any module with SliderPack data (step sequencers,
array modulators, harmonic filters, scriptnode processors). Provides indexed
access to the module's SliderPackData objects. Wraps any module implementing
ExternalDataHolder, not just C++ SliderPackProcessor subclasses.

Complex data chain:

![Slider Pack Data Chain](topology_complex-sliderpack-data-chain.svg)

  - SliderPackProcessor selects the module that owns one or more slider-pack slots.
  - SliderPackData is the complex-data handle for one slot within that module.
  - ScriptSliderPack displays or edits one selected slot in the UI.

  Use the binding properties separately:
  - processorId selects the owning processor.
  - SliderPackIndex selects which slider-pack slot inside that processor should be displayed.

  This is not the normal parameter binding path. parameterId targets processor
  parameters, while slider-pack binding uses SliderPackIndex instead.

Common mistakes:
  - Calling Synth.getSliderPackProcessor() outside onInit -- factory method
    requires object creation phase and throws a script error in callbacks.

Example:
  // Get a reference to a module's slider pack data
  const var spp = Synth.getSliderPackProcessor("ArrayModulator1");
  const var pack = spp.getSliderPack(0);

Methods (1):
  getSliderPack
