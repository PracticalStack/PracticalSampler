## stopInternalClock

**Examples:**

**Pitfalls:**
- Multiple script files can call `stopInternalClock` on different TransportHandler instances -- the clock is global, so any instance can stop it. In a complex plugin, the clock may be stopped from transport UI, preset browser, mixer controls, and preset preview systems independently. Coordinate stop/restart sequences carefully when multiple subsystems interact.
