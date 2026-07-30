// The playback-context matrix owns the complete immutable-payload fixture. Reuse it here so
// the curated DSP routing target exercises the exact Preview/Performance callback path rather
// than a separate synthetic renderer.
#define main curatedDspScopedRoutingPlaybackContextMatrix
#include "Sprint4PlaybackContextTests.cpp"
#undef main

int main()
{
    return curatedDspScopedRoutingPlaybackContextMatrix();
}
