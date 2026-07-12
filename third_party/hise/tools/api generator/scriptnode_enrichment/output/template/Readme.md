---
title: Template Nodes
factory: template
---

Template nodes are pre-built composite configurations that combine multiple scriptnode nodes into ready-made signal topologies. Rather than building common structures from scratch, drop in a template and customise it: replace placeholder nodes with your processing, connect modulation outputs to targets, and expose internal parameters as needed.

Template nodes do not appear as raw factory paths in saved networks. When you add a template, its internal structure is expanded into the network as standard nodes. The templates cover common patterns such as dry/wet mixing, feedback delay loops, mid/side processing, frequency band splitting, and soft-bypass switching.

## Nodes

| Node | Description |
|------|-------------|
| [$SN.template.bipolar_mod$]($SN.template.bipolar_mod$) | Bipolar modulation source with adjustable centre value and intensity |
| [$SN.template.dry_wet$]($SN.template.dry_wet$) | Dry/wet parallel mixer with linear crossfade |
| [$SN.template.feedback_delay$]($SN.template.feedback_delay$) | Feedback delay loop with send/receive routing in fixed 32-sample blocks |
| [$SN.template.freq_split2$]($SN.template.freq_split2$) | 2-band frequency splitter using Linkwitz-Riley crossover filters |
| [$SN.template.freq_split3$]($SN.template.freq_split3$) | 3-band frequency splitter using Linkwitz-Riley crossover filters |
| [$SN.template.freq_split4$]($SN.template.freq_split4$) | 4-band frequency splitter using Linkwitz-Riley crossover filters |
| [$SN.template.freq_split5$]($SN.template.freq_split5$) | 5-band frequency splitter using Linkwitz-Riley crossover filters |
| [$SN.template.mid_side$]($SN.template.mid_side$) | Mid/side processing framework with independent mono chains |
| [$SN.template.softbypass_switch2$]($SN.template.softbypass_switch2$) | Soft-bypass switcher with 2 processing slots |
| [$SN.template.softbypass_switch3$]($SN.template.softbypass_switch3$) | Soft-bypass switcher with 3 processing slots |
| [$SN.template.softbypass_switch4$]($SN.template.softbypass_switch4$) | Soft-bypass switcher with 4 processing slots |
| [$SN.template.softbypass_switch5$]($SN.template.softbypass_switch5$) | Soft-bypass switcher with 5 processing slots |
| [$SN.template.softbypass_switch6$]($SN.template.softbypass_switch6$) | Soft-bypass switcher with 6 processing slots |
| [$SN.template.softbypass_switch7$]($SN.template.softbypass_switch7$) | Soft-bypass switcher with 7 processing slots |
| [$SN.template.softbypass_switch8$]($SN.template.softbypass_switch8$) | Soft-bypass switcher with 8 processing slots |
