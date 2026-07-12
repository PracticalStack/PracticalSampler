---
title: Language Reference
description: Reference for all languages used in HISE — scripting, DSP, styling, and text formatting

guidance:
  summary: >
    Index page for the HISE language reference section. Groups languages into
    three categories: DSP languages (SNEX, Faust, RNBO, C++ DSP Nodes) for
    writing custom signal processing inside scriptnode, UI languages (HiseScript,
    CSS) for scripting and styling the plugin interface, and additional languages
    (Regex, Markdown) used in specific API contexts. Each category includes a
    decision table to help users choose the right language for their task.
  concepts:
    - language reference
    - HiseScript
    - SNEX
    - Faust
    - RNBO
    - C++ DSP Nodes
    - CSS
    - Regex
    - Markdown
  complexity: beginner
---

HISE uses several languages, each designed for a specific part of plugin development. This section documents the syntax, features, and HISE-specific behaviour of each language.


## DSP Languages

These languages are used inside scriptnode to write custom signal processing code. Each compiles to native machine code — they differ in syntax, iteration speed, and what libraries they can access.

| If you... | Use |
| --- | --- |
| Want to write custom DSP with instant compilation in the HISE IDE | [SNEX]($LANG.snex$) |
| Want concise functional DSP syntax or need algorithms from the Faust standard library | [Faust]($LANG.faust$) |
| Need JUCE framework access, existing scriptnode building blocks, or hardcoded module loading | [C++ DSP Nodes]($LANG.cpp-dsp-nodes$) |
| Already work in Max/MSP and want to bring patches into HISE | [RNBO]($LANG.rnbo$) |

**[SNEX]($LANG.snex$)** — HISE's JIT-compiled C++ subset. Compiles instantly in the HISE IDE with no external tooling. The callback API is identical to C++ DSP Nodes, so porting to full C++ later is a direct copy-paste.

**[Faust]($LANG.faust$)** — A functional DSP language with a comprehensive library of filters, oscillators, reverbs, and physical models. Compiles automatically on save. No access to external data slots (Tables, SliderPacks, AudioFiles).

**[C++ DSP Nodes]($LANG.cpp-dsp-nodes$)** — Full C++ with access to the JUCE framework, existing scriptnode node classes, and hardcoded module loading. Requires an external IDE and a DLL compile step, but has no restrictions on what you can do.

**[RNBO]($LANG.rnbo$)** — Import DSP patches from Cycling '74's RNBO (a subset of Max/MSP). Requires a manual export→compile cycle for every change. Best suited if Max/MSP is already part of your workflow.

> [!Tip:Start with SNEX, graduate to C++] If you're writing custom DSP from scratch, start with [SNEX]($LANG.snex$) for the fast iteration loop. When you outgrow the language subset — you need existing nodes, JUCE utilities, or hardcoded module loading — port to [C++ DSP Nodes]($LANG.cpp-dsp-nodes$). The callback API is the same, so the transition is straightforward.


## UI Languages

These languages control the plugin interface — scripting logic, component behaviour, and visual styling.

| If you... | Use |
| --- | --- |
| Need to script UI logic, react to MIDI/timer events, or control modules from code | [HiseScript]($LANG.hisescript$) |
| Want to style component appearance declaratively with selectors, pseudo-classes, and transitions | [CSS]($LANG.css$) |

**[HiseScript]($LANG.hisescript$)** — HISE's main scripting language. JavaScript syntax adapted for realtime audio safety. Used for UI logic, MIDI processing, module control, and custom graphics via paint routines. Every HISE project uses HiseScript.

**[CSS]($LANG.css$)** — A declarative alternative to paint routines for styling UI components. Pseudo-classes and transitions handle hover/active/checked states automatically. Each component type documents its supported CSS selectors and properties on its own reference page.


## Additional Languages

These are standard languages used in specific HISE API contexts. HISE does not modify them — the reference pages document where they appear and how to use them.

| If you... | Use |
| --- | --- |
| Need to select components, modules, or samples by name pattern | [Regex]($LANG.regex$) |
| Need to display formatted text in your plugin UI or on the HISE forum | [Markdown]($LANG.markdown$) |

**[Regex]($LANG.regex$)** — Standard ECMAScript regular expressions. Used by `Content.getAllComponents`, `Sampler.selectSounds`, `Engine.getRegexMatches`, and other API methods for pattern matching.

**[Markdown]($LANG.markdown$)** — A lightweight markdown subset rendered by the MarkdownRenderer scripting API class, the ScriptDynamicContainer TextBox component, and the HISE forum.
