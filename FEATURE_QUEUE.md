# Feature queue

Delivered 28 October 2026: camera-track status now reports pause, scrub, playback, and completion correctly. Pause/scrub reports the current recorded time and retains the collision-disabled notice.

Final round, 28 October 2026: orbit buttons, zoom, keyboard and pointer movement now replace stale playback status with orbit status. Actual handler regression and browser checks cover playback interruption.

Remaining work (daily feature automation stopped):
- Add explicit geometry-budget diagnostics for engine integration.
- Perform actual Unreal Engine compile, input, rendering and packaging acceptance using docs/unreal-validation.md. This needs an installed engine and remains unverified.

Portable and browser checks do not establish engine or hardware behavior.
