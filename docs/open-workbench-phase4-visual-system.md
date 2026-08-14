# Open Workbench Phase 4: light visual system

Phase 4 is implemented as a token-driven restyle of the authoring workspace. It intentionally preserves the Phase 0–3 component structure, layout geometry, component IDs, interaction model, authoring callbacks, and session state.

## Shared visual contract

`app/src/shared/authoring/OpenWorkbenchVisualSystem.h` is the source of truth for authoring colors, borders, radii, focus treatment, and dense desktop metrics. Custom authoring paint code must use its semantic roles instead of adding component-local RGB values.

The principal roles are:

| Role | Value | Use |
| --- | --- | --- |
| `shell` | `#E9ECE8` | Standalone/plugin shell and authoring perimeter |
| `surface` / `surfaceRaised` | `#FFFFFF` | Editors, panes, fields, and idle controls |
| `surfaceSubtle` | `#F7F8F5` | Toolbars, list rows, splitter, and recessed chrome |
| `mapSurface` | `#FBFAF5` | Zone Map and waveform data surfaces |
| `text` | `#273035` | Primary graphite text |
| `textMuted` | `#5F686B` | Secondary metadata |
| `border` / `borderStrong` | `#BAC2C1` / `#879395` | One-pixel structure and higher-priority boundaries |
| `selection` | `#B6531D` | Authored selection and selected controls |
| `focus` | `#28658F` | Keyboard focus, independent of selection |
| `information` | `#2B76B7` | Informational state |
| `modulation` | `#28786F` | Modulation and related valid state |
| `success` | `#4F7E3B` | Healthy/complete state |

The approved orange was darkened from the early `#D76828` reference swatch so white selected-state text exceeds 4.5:1 contrast. Controls use a 2px radius, panels use a 3px radius, and structural boundaries use 1px strokes. Adjacent content regions remain rectangular and do not add shadows.

## State treatment

- Idle controls use a neutral raised surface with graphite text.
- Hover uses `surfaceHover`; pressed state moves toward `selectionHover`.
- Toggle/tab selection uses orange with white text.
- Keyboard focus always uses the blue focus ring and remains visible on selected controls.
- Disabled colors are blended toward the light surface and disabled text uses a distinct neutral.
- Warning, error, busy/information, and success banners are low-chroma semantic tints with a structural border.
- Group tints are stable, muted data colors. They never replace labels, selection outlines, or focus.
- Crossfade blue/orange are documented data-bearing exceptions; their gradients encode direction.

## Component coverage

The token source now drives the authoring shell and control LookAndFeel, summary strip, contextual inspector primitives, workbench splitter, repeated-structure lists, waveform, Zone Map, minimap, stable group palette, pop-up menus, tooltips, and scrollbars. Standalone and VST3 containers receive the neutral shell and tab treatment; the Perform workspace itself is deliberately unchanged.

## Qualification

`drs.open_workbench.phase4` checks:

- AA/enhanced contrast targets for primary text, metadata, selected-state text, and keyboard focus;
- independence of selection, focus, information, and modulation roles;
- 2–3px geometry, one-pixel borders, and shared dense row/control metrics;
- deterministic group tinting;
- rendered light shell, work surface, Zone Map toolbar, map focus, and waveform surface;
- neutral idle, orange selected, and blue focused workbench states;
- removal of the legacy dark dependency from the summary and map.

The Phase 0–3 Open Workbench targets and existing authoring UI, mapping, macro/routing, repeated-structure, and performance UI targets remain the regression boundary for this phase. Full host/high-DPI/manual qualification remains Phase 5 work.
