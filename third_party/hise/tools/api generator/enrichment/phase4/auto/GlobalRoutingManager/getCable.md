Returns a [GlobalCable]($API.GlobalCable$) reference for the given cable ID. If no cable with that ID exists, it is created on demand. Use `/`-prefixed IDs for cables that should participate in OSC routing; other IDs work for internal routing only.

The returned object provides methods for reading, writing, and registering callbacks on the cable's value. Multiple calls with the same ID return separate wrapper objects that share the same underlying cable, so you can safely obtain references from different parts of your script.
