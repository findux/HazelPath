# Simulation visibility checks

Run from the repository root with Visual Studio C++ tools installed:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tests\run_simulation_visibility_tests.ps1
```

The test builds the actual SimulationController, MotionInterpolator and Toolpath sources without an OpenGL context. It checks play/pause, command stepping entry points, forward/backward seek, reverse playback to zero, completion/restart, operation changes, dwell, rapid/feed draw ranges, partial tessellated segments, helix sampling, toolpath replacement, empty paths and 10,000 seeks in a 200,000-command path. Build artifacts go to a unique temporary directory.

Manual UI checks (not covered by the headless test):

- Play, pause and step using buttons and keyboard; the visible line ends at the current tessellated segment progress.
- Seek 0%, 25%, 50%, 100%, then back to 25% and 0%; check that future lines disappear.
- Change speed in both directions; reverse playback reaching zero must leave the simulation path empty.
- Stop/F7 returns to the full normal toolpath view; Play/F5 activates progressive visibility again.
- Toggle rapid visibility; verify existing gray/green/blue colors, camera, fitting and picking.
- Load another program and verify that no old geometry or visibility boundary remains.
- Profile large-path rendering separately: the seek benchmark does not measure GPU frame rate.

The vertex boundary cache is rebuilt with the static meshes and cleared on import failure. The cumulative time cache is rebuilt by setToolpath and feed-rate setters. Picking and camera fitting continue to use full geometry. Partial arcs follow the existing tessellated chords, so their endpoint can differ slightly from the analytically sampled cone position within the existing tessellation tolerance.
