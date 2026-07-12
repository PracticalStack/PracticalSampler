---
title: Effect Slot
moduleId: SlotFX
type: Effect
subtype: MasterEffect
tags: [utility, routing]
builderPath: b.Effects.SlotFX
screenshot: /images/v2/reference/audio-modules/slotfx.png
cpuProfile:
  baseline: negligible
  polyphonic: false
  scalingFactors: []
seeAlso:
  - { id: EmptyFX, type: companion, reason: "The default placeholder loaded when the slot is empty or cleared" }
forumReferences:
  - id: 1
    title: "setBypassed() does not work on SlotFX scripting references"
    summary: "Call setBypassed() via a second reference obtained with Synth.getEffect() instead — both references target the same physical module."
    topic: 5947
  - id: 2
    title: "SlotFX state is not saved in user presets by default"
    summary: "Call Engine.addModuleStateToUserPreset(moduleId) in onInit to include the loaded effect and its parameters in user preset data."
    topic: 13353
  - id: 3
    title: "Filter effect type is excluded from the Effect Slot dropdown"
    summary: "The built-in Filter module cannot be loaded into an Effect Slot via setEffect(); use a Script FX wrapping filter behaviour as a workaround."
    topic: 2536
commonMistakes:
  - title: "No crossfade on effect swap"
    wrong: "Expecting a crossfade when swapping effects in the slot during playback"
    right: "The swap is instantaneous with no crossfade - plan for a brief audio interruption"
    explanation: "Effect swapping happens at the next processing boundary. There is no built-in crossfade between the old and new effects."
  - title: "Can't nest slots or load polyphonic"
    wrong: "Trying to load a polyphonic effect or another Effect Slot into the slot"
    right: "Only master effects and monophonic effects are allowed, excluding Route Effect and nested Effect Slots"
    explanation: "The slot restricts child types to prevent incompatible or recursive configurations."
  - title: "setEffect() requires the type name, not the instance name"
    wrong: "Passing the module's instance name from the module tree to setEffect()"
    right: "Pass the effect type name as it appears in the slot dropdown (e.g. 'SimpleReverb', 'PhaseFX')"
    explanation: "setEffect() expects the module type name, not the instance name you set in the module tree. Passing an instance name silently fails."
  - title: "Multi-channel effects cannot be loaded"
    wrong: "Loading a HardcodedMasterFX with more than 2 channels into an Effect Slot"
    right: "Only 2-channel effects are supported in the Effect Slot"
    explanation: "Effect Slots only support 2-channel processing. A HardcodedMasterFX built on a scriptnode graph with more than 2 channels will crash on load."
  - title: "Controls not updated after slot swap"
    wrong: "Expecting UI controls to automatically show the new effect's parameter values after calling setEffect()"
    right: "Re-read attribute values after the swap, using a short delay or a broadcaster callback"
    explanation: "After setEffect(), the newly loaded effect's parameters are not automatically pushed to connected UI controls. A timing issue means the effect may not be fully loaded when the callback fires."
llmRef: |
  Effect Slot (MasterEffect)

  A container that hosts a single swappable child effect. The hosted effect can be changed at runtime, making it useful for dynamic signal chains where the user selects from a list of effects. When empty, audio passes through unchanged.

  Signal flow:
    audio in -> [if effect loaded] child effect processing -> audio out
    audio in -> [if empty] -> audio out (passthrough)

  CPU: negligible (wrapper overhead only), monophonic. Actual CPU depends on the hosted effect.

  Parameters: none (all parameters belong to the hosted child effect).

  Allowed child effects: all master effects and monophonic effects except Route Effect and Effect Slot (no nesting).

  When to use:
    Building dynamic effect chains where the user can swap between different effects at runtime. Commonly used with scripting to create switchable effect racks.

  Common mistakes:
    No crossfade on swap - the transition is instantaneous.
    Only master/monophonic effects allowed, no polyphonic effects or nested slots.
    setEffect() requires the type name (e.g. "SimpleReverb"), not the instance name from the module tree.
    HardcodedMasterFX with more than 2 channels cannot be loaded into an Effect Slot.
    UI controls are not automatically updated after a slot swap - re-read attribute values manually.

  Tips:
    Use Engine.addModuleStateToUserPreset() to persist the loaded effect and its parameters across user presets.
    setBypassed() does not work on SlotFX references - use Synth.getEffect() to get a second reference.
    The Filter effect type is excluded from the Effect Slot dropdown and cannot be loaded via setEffect().

  See also:
    companion Empty - default placeholder when slot is empty
---

::category-tags
---
tags:
  - { name: utility, desc: "Modules for analysis, placeholders, or structural purposes without audio processing" }
  - { name: routing, desc: "Modules that forward, distribute, or proxy signals or events across the module tree" }
---
::

![Effect Slot screenshot](/images/v2/reference/audio-modules/slotfx.png)

The Effect Slot hosts a single child effect that can be swapped at runtime. It is the standard building block for dynamic signal chains where the user selects from a list of available effects. When no effect is loaded, audio passes through unchanged.

The slot accepts all master effects and monophonic effects, excluding Route Effect and other Effect Slots (no nesting). MIDI events, bypass state, and monophonic voice lifecycle are all forwarded to the hosted effect. The slot itself has no parameters - all controls belong to the child.

### Effect Swapping

When an effect is swapped, the transition is instantaneous at the next processing boundary. There is no crossfade between the old and new effects, which may produce a brief audio discontinuity if the swap occurs during playback.

Two Effect Slots can exchange their hosted effects atomically using the swap operation, which is useful for reordering effects in a dynamic chain.

### Empty State

An empty slot always contains an Empty effect internally rather than being truly null. Clearing the slot replaces the current effect with a fresh Empty instance.

### Bypass Delegation

Bypass is fully delegated to the hosted effect. Bypassing the Effect Slot forwards the bypass state to whatever effect is currently loaded.

:::{.warning}
`setBypassed()` does not work when called on a SlotFX scripting reference [1]($FORUM_REF.5947$). As a workaround, obtain a second reference to the same module using `Synth.getEffect()` and call `setBypassed()` on that reference instead.
:::

### Persisting State Across Presets

By default, user presets do not save which effect is loaded in an Effect Slot or its parameter values. Call `Engine.addModuleStateToUserPreset(moduleId)` in onInit for each Effect Slot to include its full state in user preset data [2]($FORUM_REF.13353$).

### Excluded Effect Types

The built-in Filter module is excluded from the Effect Slot type list and cannot be loaded via `setEffect()` [3]($FORUM_REF.2536$). Use a Script FX wrapping the desired filter behaviour as a workaround.

**See also:** $MODULES.EmptyFX$ -- The default placeholder loaded when the slot is empty or cleared
