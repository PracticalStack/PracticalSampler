Attaches the broadcaster to a radio button group, firing whenever a button in the group is clicked. The broadcaster receives the zero-based index of the selected button within the group (using the same order as the component list). On attachment, existing listeners immediately receive the index of the currently active button.

This is the only attachment mode that supports bidirectional communication: sending a message via dot-assignment (`bc.buttonIndex = 2`) or `sendSyncMessage` also changes the currently active button in the radio group.

This is commonly used for page-handling logic: attach to a radio group, then use `addComponentPropertyListener` to show or hide page panels based on the selected index.

> [!Warning:RadioGroupIndex must be greater than zero] The `radioGroupIndex` must be a positive integer (greater than zero). Passing `0` produces an error.
