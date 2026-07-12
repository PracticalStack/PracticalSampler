Creates an array of Path objects representing the envelope of a partial parameter for a given harmonic, one Path per audio channel. Internally calls `createEnvelopes()` to get the raw buffer data, then converts each buffer to a downsampled Path (approximately 200 points) suitable for UI display. The path is clipped to the valid range for the parameter.

Pass `0` for the harmonic index to get the envelope for the fundamental. The file must have been previously analysed.
