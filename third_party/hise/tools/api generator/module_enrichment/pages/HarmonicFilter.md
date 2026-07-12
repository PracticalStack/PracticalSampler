---
title: Harmonic Filter
moduleId: HarmonicFilter
type: Effect
subtype: VoiceEffect
tags: [filter]
builderPath: b.Effects.HarmonicFilter
screenshot: /images/v2/reference/audio-modules/harmonicfilter.png
cpuProfile:
  baseline: low
  polyphonic: true
  scalingFactors:
    - { parameter: NumFilterBands, effect: "CPU scales linearly with band count: 1 band = low, 4 bands = medium, 16 bands = high" }
seeAlso:
  - { id: PolyphonicFilter, type: alternative, reason: "General-purpose polyphonic filter with standard filter modes. Use when you need a conventional LP/HP/BP filter per voice rather than harmonic resonance shaping." }
  - { id: HarmonicFilterMono, type: alternative, reason: "Monophonic variant that tracks the last played note instead of maintaining independent filter state per voice. Lower CPU cost but no per-voice harmonic separation." }
  - { id: PolyshapeFX, type: companion, reason: "Polyphonic waveshaper that can be placed before the Harmonic Filter to add harmonics for the filter to shape." }
commonMistakes:
  - title: "Filter frequencies are fixed at note-on"
    wrong: "Expecting the filter frequencies to follow pitch bend or pitch modulation after the note starts"
    right: "The harmonic frequencies are fixed at note-on and do not update during the voice's lifetime"
    explanation: "The filter bank tuning is set once from the MIDI note frequency (plus SemitoneTranspose). Pitch bend, glide, or pitch modulation applied after note-on will not retune the filters."
  - title: "Band count scales CPU with voices"
    wrong: "Setting NumFilterBands to 16 on a high-polyphony patch and wondering about CPU spikes"
    right: "Start with fewer bands and increase only as needed. High notes automatically use fewer bands due to Nyquist clamping."
    explanation: "Each band adds a peak filter per sample per voice. With 16 bands and 16 voices, that is 256 filters running simultaneously. The automatic Nyquist clamping helps for high notes but low notes will use all requested bands."
  - title: "Changing bands causes transient"
    wrong: "Changing NumFilterBands during playback and expecting a smooth transition"
    right: "Changing the band count resets all filter states, which may cause a brief transient"
    explanation: "The filter states are cleared when the band count changes. Avoid automating this parameter during sustained notes."
forumReferences:
  - id: 1
    title: "Slider pack components can be promoted to the interface"
    summary: "Pack A, B, and Mix can be promoted to the plugin interface for custom layout; FilterDisplay FloatingTile does not support this module."
    topic: 671
  - id: 2
    title: "FilterDisplay FloatingTile does not support Harmonic Filter"
    summary: "The FilterDisplay content type cannot render the Harmonic Filter; build the UI manually by promoting the slider pack components."
    topic: 671
  - id: 3
    title: "Polyphonic version is CPU-intensive at high voice counts"
    summary: "The polyphonic Harmonic Filter scales linearly with voice count — the author describes it as 'quite a CPU hog, especially in the polyphonic version' with high band counts."
    topic: 671
customEquivalent:
  approach: scriptnode
  moduleType: HardcodedFX
  complexity: complex
  description: "Requires a bank of peak EQ nodes with per-voice frequency calculation from the MIDI note, slider pack data routing, and A/B crossfade logic. Non-trivial due to the dynamic band count and harmonic frequency tracking."
llmRef: |
  HarmonicFilter (VoiceEffect, polyphonic)

  Per-voice bank of peak EQ filters tuned to integer harmonics (f, 2f, 3f, ...) of each voice's MIDI note frequency. Two slider pack configurations (A and B) define per-band gain in dB, crossfaded via the CrossfadeValue parameter. Filters run in series. No dry/wet mix - output is fully filtered.

  Signal flow:
    voice audio in -> series peak filter bank (1-16 bands at harmonic frequencies) -> voice audio out
    Frequencies set once at note-on from MIDI note + SemitoneTranspose. Bands above Nyquist safety limit (0.4 * sampleRate) are automatically excluded.

  CPU: low (1 band) to high (16 bands), polyphonic. Scales linearly with band count and voice count.

  Parameters:
    NumFilterBands (1-16 in powers of two, default 1) - number of active harmonic bands
    QFactor (4-48, default 12) - filter resonance, uniform across all bands. Higher = narrower peaks.
    CrossfadeValue (0-100%, default 50%) - blend between slider pack A (0%) and B (100%)
    SemitoneTranspose (-24 to +24, default 0) - shifts the base frequency and entire harmonic series

  Modulation chains:
    X-Fade Modulation - scales the CrossfadeValue parameter per block. Allows dynamic morphing between A and B configurations.

  Slider packs:
    Pack A: per-band gain in dB (-24 to +24) for each harmonic
    Pack B: alternate per-band gain in dB (-24 to +24)
    Pack Mix: read-only display showing the interpolated result

  When to use:
    Harmonic resonance shaping, formant filtering, spectral sculpting that tracks the played note. Each voice gets its own filter bank tuned to that voice's pitch.

  Common mistakes:
    Filter frequencies are fixed at note-on - pitch bend does not retune filters.
    16 bands with high polyphony is CPU-intensive.
    Changing NumFilterBands during playback resets filter states (transient).

  Custom equivalent:
    scriptnode HardcodedFX: complex - requires peak EQ bank with MIDI frequency tracking and A/B crossfade logic.

  See also:
    PolyphonicFilter - general-purpose per-voice filter (LP/HP/BP modes, not harmonic tracking)
    HarmonicFilterMono - monophonic variant, tracks last played note only
    PolyshapeFX - polyphonic waveshaper, useful before HarmonicFilter to generate harmonics
---

::category-tags
---
tags:
  - { name: filter, desc: "Effects that shape the frequency spectrum of the audio signal" }
---
::

![Harmonic Filter screenshot](/images/v2/reference/audio-modules/harmonicfilter.png)

A polyphonic bank of peak EQ filters tuned to the integer harmonics of each voice's played note. When a note starts, the module calculates harmonic frequencies (f, 2f, 3f, ...) from the MIDI note and sets a series of peak filters at those frequencies. The per-band gain values are defined by two slider pack configurations (A and B), which can be crossfaded to morph between different harmonic profiles in real time.

Each voice maintains its own independent filter bank, so chords and polyphonic passages receive per-note harmonic shaping. The crossfade between slider packs A and B interpolates the gain values before filtering - only one filter bank runs per voice, keeping the crossfade CPU-neutral. There is no dry/wet mix; the output is the fully filtered signal. At high voice counts and band counts the CPU cost can be prohibitive. [3]($FORUM_REF.671$)

## Signal Path

::signal-path
---
glossary:
  parameters:
    NumFilterBands:
      desc: "Number of active harmonic peak filter bands (1, 2, 4, 8, or 16)"
      range: "1, 2, 4, 8, 16"
      default: "1"
    QFactor:
      desc: "Filter resonance applied uniformly to all bands. Higher values produce narrower peaks."
      range: "4 - 48"
      default: "12"
    CrossfadeValue:
      desc: "Blend position between slider pack A and B gain configurations"
      range: "0 - 100%"
      default: "50%"
    SemitoneTranspose:
      desc: "Shifts the base frequency and entire harmonic series in semitones"
      range: "-24 - +24 st"
      default: "0"
  functions:
    computeHarmonics:
      desc: "Calculates integer harmonic frequencies (f, 2f, 3f, ...) from the transposed note frequency, excluding bands above Nyquist safety limit"
    interpolateGains:
      desc: "Linearly interpolates per-band dB gain values between slider pack A and B using the crossfade position"
    peakFilter:
      desc: "Peak EQ filter applied per-sample. All bands are processed in series - each filter modifies the signal in place before the next."
  modulations:
    XFadeModulation:
      desc: "Scales the crossfade position between A and B configurations per block"
      scope: "per-voice"
---

```
// Harmonic Filter - per-voice harmonic peak EQ bank
// stereo in -> stereo out (per voice)

onNoteOn(note) {
    baseFreq = noteToFrequency(note + SemitoneTranspose)
    harmonicFreqs = computeHarmonics(baseFreq, NumFilterBands)
}

process(left, right) {
    // Crossfade between A and B gain configurations
    xfade = CrossfadeValue * XFadeModulation
    bandGains = interpolateGains(sliderPackA, sliderPackB, xfade)

    // Apply peak filters in series at each harmonic frequency
    for each band in NumFilterBands {
        left, right = peakFilter(left, right, harmonicFreqs[band], QFactor, bandGains[band])
    }
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Filter Bank
    params:
      - { name: NumFilterBands, desc: "Number of active harmonic filter bands. Each band targets a successive integer harmonic of the voice's note frequency. Changing this during playback resets all filter states.", range: "1 Filter Band, 2 Filter Bands, 4 Filter Bands, 8 Filter Bands, 16 Filter Bands", default: "1 Filter Band" }
      - { name: QFactor, desc: "Resonance of all peak filters, applied uniformly across every band. Higher values produce narrower, more pronounced peaks at each harmonic. Lower values produce broader, more subtle shaping.", range: "4 - 48", default: "12" }
  - label: A/B Crossfade
    params:
      - { name: CrossfadeValue, desc: "Blend position between slider pack A and slider pack B gain configurations. At 0% only pack A values are used, at 100% only pack B. Intermediate values linearly interpolate the per-band gains before filtering.", range: "0 - 100%", default: "50%" }
  - label: Tuning
    params:
      - { name: SemitoneTranspose, desc: "Shifts the base frequency of the harmonic series in semitones. Positive values shift all filter frequencies up, negative values shift them down. Applied at voice start.", range: "-24 - +24 st", default: "0" }
---
::

## Modulation Chains

::modulation-table
---
chains:
  - { name: "X-Fade Modulation", desc: "Scales the CrossfadeValue parameter to dynamically morph between slider pack A and B configurations. Applied once per audio block. Allows envelopes, LFOs, or other modulators to animate the harmonic profile over time.", scope: "per-voice", constrainer: "*" }
---
::

### Frequency Tracking

The harmonic filter frequencies are set once at note-on and remain fixed for the voice's lifetime. Pitch bend, glide, and pitch modulation do not retune the filter bank after the initial calculation.

Bands whose frequency would exceed a safety limit below the Nyquist frequency are automatically excluded. For high notes, the actual number of active bands may be fewer than the NumFilterBands setting. For example, a note at 4000 Hz at a 44100 Hz sample rate will only use approximately 4 bands regardless of the setting.

### Slider Pack Data

The module uses three slider packs internally: pack A and pack B hold the editable per-band gain values in dB (-24 to +24), and a third read-only pack displays the interpolated mix result. The crossfade interpolates gain values before filtering - it does not run two separate filter banks. The three slider pack components (A, B, Mix) can be promoted to the plugin interface for custom layout and colouring. [1]($FORUM_REF.671$)

The FilterDisplay FloatingTile does not support this module. Build the filter UI manually using promoted slider pack components. [2]($FORUM_REF.671$)

**See also:** $MODULES.PolyphonicFilter$ -- General-purpose polyphonic filter with standard filter modes. Use when you need a conventional LP/HP/BP filter per voice rather than harmonic resonance shaping., $MODULES.HarmonicFilterMono$ -- Monophonic variant that tracks the last played note instead of maintaining independent filter state per voice. Lower CPU cost but no per-voice harmonic separation., $MODULES.PolyshapeFX$ -- Polyphonic waveshaper that can be placed before the Harmonic Filter to add harmonics for the filter to shape.
