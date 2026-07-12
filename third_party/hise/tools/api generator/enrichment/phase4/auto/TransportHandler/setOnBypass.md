Registers a callback that fires when the plugin's bypass state changes. This callback is always asynchronous (no sync parameter). The callback receives a boolean indicating whether the plugin is bypassed. Fires immediately upon registration with the current bypass state.

The bypass detection uses a watchdog timer - if the audio render callback stops being called for 10 audio buffers, the plugin is assumed bypassed. This has a delay of about 100ms at 512 samples, but works across DAWs that lack a standard bypass API.
