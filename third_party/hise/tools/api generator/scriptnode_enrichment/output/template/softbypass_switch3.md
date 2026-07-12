---
title: Soft Bypass Switch 3
description: "A click-free 3-way signal path switch using soft-bypassed containers."
factoryPath: template.softbypass_switch3
factory: template
polyphonic: false
tags: [template, switch, bypass, crossfade]
cpuProfile:
  baseline: low
  polyphonic: false
  scalingFactors:
    - { parameter: "active path contents", impact: "variable", note: "CPU cost depends on the processing inside the active path" }
seeAlso:
  - { id: "template.softbypass_switch2", type: alternative, reason: "2-way variant" }
  - { id: "template.softbypass_switch4", type: alternative, reason: "4-way variant" }
  - { id: "template.softbypass_switch8", type: alternative, reason: "8-way variant (maximum)" }
  - { id: "container.soft_bypass", type: companion, reason: "The underlying bypass container used internally" }
  - { id: "control.xfader", type: companion, reason: "The switch controller used internally" }
  - { id: "container.branch", type: alternative, reason: "Index-based branching without soft bypass crossfade" }
commonMistakes:
  - title: "Expecting immediate hard switching"
    wrong: "Expecting hard switching with an immediate transition"
    right: "The switch uses soft bypass crossfading -- there is a brief overlap during transitions."
    explanation: "Each path crossfades over a 20ms smoothing period by default. During the transition, both the outgoing and incoming paths process simultaneously. This prevents clicks but means the switch is not instantaneous."
llmRef: |
  template.softbypass_switch3

  A pre-built 3-way signal path switch with click-free crossfading. Uses an xfader in Switch mode to activate one of three soft_bypass containers at a time.

  Signal flow:
    Switch param -> xfader -> bypass state for sb1, sb2, sb3
    audio in -> sb1 -> sb2 -> sb3 -> audio out
    Only one path is active; the others pass audio through unmodified.

  CPU: low, monophonic
    Baseline overhead is minimal. Actual cost depends on the processing inside the active path. During transitions, two paths process briefly.

  Parameters:
    Switch: 0 - 2 (step 1, default 0.0) -- selects the active signal path

  When to use:
    Use when you need to switch between three different processing chains without audible clicks. Replace the dummy nodes inside each soft_bypass container with your processing. For hard switching without crossfade, use container.branch instead.

  Common mistakes:
    Expecting an instant transition -- the switch crossfades over 20ms by default.

  See also:
    [alternative] template.softbypass_switch2 -- 2-way variant
    [alternative] template.softbypass_switch4 -- 4-way variant
    [alternative] container.branch -- index-based branching without soft bypass
    [companion] container.soft_bypass -- the underlying bypass container
    [companion] control.xfader -- the switch controller used internally
---

This template provides a click-free 3-way signal path switch. It contains three [soft_bypass]($SN.container.soft_bypass$) containers controlled by an [xfader]($SN.control.xfader$) in Switch mode. Setting the Switch parameter activates one container and bypasses the others, with a smooth crossfade during the transition to prevent clicks.

Each container holds a placeholder dummy node. Replace these with your actual processing chains to create three switchable signal paths. Only the active path applies its processing; the bypassed paths pass audio through unmodified. During a switch transition, both the outgoing and incoming paths process simultaneously for a brief crossfade period (20ms by default).

## Signal Path

::signal-path
---
glossary:
  parameters:
    Switch:
      desc: "Selects the active signal path"
      range: "0 - 2"
      default: "0.0"
  functions:
    selectPath:
      desc: "Routes the Switch value to bypass states, activating one container and bypassing all others"
---

```
// template.softbypass_switch3 - 3-way click-free switch
// audio in -> audio out

dispatch(input) {
    selectPath(Switch)           // activate one, bypass the rest

    path[0].process(input)       // Switch=0: active
    path[1].process(input)       // Switch=1: active
    path[2].process(input)       // Switch=2: active
    // only the selected path modifies the signal
}
```

::

## Parameters

::parameter-table
---
groups:
  - params:
      - { name: Switch, desc: "Selects which signal path is active. Each integer value activates the corresponding container and bypasses the others.", range: "0 - 2", default: "0.0" }
---
::

During a switch transition, both the outgoing and incoming paths process simultaneously for the crossfade duration, briefly doubling the processing load. The default crossfade time is 20ms per container -- you can adjust the SmoothingTime property on each [soft_bypass]($SN.container.soft_bypass$) container individually. Setting SmoothingTime to 0 makes the switch behave as a hard bypass (immediate, may click).

**See also:** $SN.template.softbypass_switch2$ -- 2-way variant, $SN.template.softbypass_switch4$ -- 4-way variant, $SN.template.softbypass_switch8$ -- 8-way variant (maximum), $SN.container.soft_bypass$ -- the underlying bypass container, $SN.control.xfader$ -- the switch controller used internally, $SN.container.branch$ -- index-based branching without soft bypass crossfade
