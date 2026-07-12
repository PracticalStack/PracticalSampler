Installs a custom sort function that controls the order of plugin parameters as exposed to the DAW host. The function receives two JSON objects and must return a negative number (first before second), zero (equal), positive (second before first), or `undefined` to fall back to the default order for that pair. Pass a non-function value to reset to default sorting.

Each parameter object has these properties:

| Property | Type | Description |
|----------|------|-------------|
| `type` | Number | 0 = Macro, 1 = Custom automation, 2 = Script control, 3 = NKS |
| `parameterIndex` | Number | Index in the full default-sorted list |
| `typeIndex` | Number | Index within its type category |
| `name` | String | Display name |
| `group` | String | Plugin parameter group (empty string if ungrouped) |

The default sort order groups by type (macros first, then custom automation, then script controls), and within each type by order of definition.

> [!Warning:Parameter groups override sort order] If you use parameter groups, the host requires ungrouped parameters first, followed by grouped parameters. This grouping overrides any custom sort order you define, so adding group names to an existing project may change the parameter order unexpectedly.
