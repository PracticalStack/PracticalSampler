Creates an array of audio-rate Buffer objects representing the envelope of a partial parameter for a given harmonic, one Buffer per audio channel. Each buffer contains the envelope sampled at the original file's sample rate. The file must have been previously analysed.

Pass `0` for the harmonic index to get the envelope for the fundamental. The `"rootFrequency"` parameter uses the F0 estimate internally, while all other parameters trigger an automatic preparation step (channelisation, collation, sifting, distillation, sorting) before extraction.
