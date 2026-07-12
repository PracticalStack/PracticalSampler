Enables or disables queue mode. When enabled, two behaviours change: change detection is bypassed (identical values are still dispatched), and async coalescing is bypassed (every async send posts an independent job rather than coalescing into one).

Without queue mode, rapid async sends are coalesced -- only the latest value reaches listeners. For example, `bc.sendAsyncMessage([0]); bc.sendAsyncMessage([1]);` delivers only the value `1`. With queue mode enabled, both values are delivered in order.

Several `attachTo*` methods automatically enable queue mode because their sources may fire multiple times before the scripting thread processes. Disabling queue mode after such an attachment may cause missed events.
