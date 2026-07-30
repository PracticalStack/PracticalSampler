# Curated Stereo Delay v1

`drs.stereoDelay` v1 uses two preallocated 2-second, 96 kHz-capacity rings with linear fractional
reads and no callback allocation. Free time is 1–2000 ms; sync time uses 120 BPM when host tempo
is absent or invalid. Divisions span 1/16 to four beats, feedback clamps to 0.95, and reset/panic
clears rings immediately. A normal release retains a bounded two-second retirement tail after the
last non-silent delay input. Tempo or parameter updates never reallocate memory.

Ping-pong crosses feedback channels; tone filters feedback; width is reserved in the stable v1
surface and currently preserves the stereo ring layout. Changing any algorithm behavior requires v2.
