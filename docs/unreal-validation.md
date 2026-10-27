# Unreal acceptance remains pending

No Unreal Editor or Build Tool is installed on the task Mac. The task did not install an engine, accept an Epic agreement, compile Unreal source, open the editor, render an engine frame, cook content, package a game, or measure frame performance. Portable C++ and browser evidence must not be relabeled as engine evidence.

On an existing licensed installation, record the exact engine/toolchain versions and perform these checks:

1. Select the engine for `FractalFPV.uproject` and generate project files. Compile `FractalFPVEditor`. Confirm the runtime ProceduralMeshComponent plugin and module load without UHT/C++ errors.
2. Open the default Entry map and press Play. Confirm `FPVGameMode` is active and spawns one fractal actor, the outside pawn, HUD and dynamic lights. Verify visible geometry with the built-in material. Save any edited map into the project's own Content folder rather than overwrite an engine map.
3. Inspect log counts at depths 0–3. Compare against the portable reports. Verify that an invalid depth/size clears the mesh and reports an error. Check front faces and shading inside and outside holes; the native unit-box winding sample must be nondegenerate.
4. Check centimetre scale against a known editor ruler reference. Keep actor scale at 1. Position conversion is ×100; normals and UVs are not scaled.
5. Test WASD, mouse/arrows, Space/Ctrl and reset. Confirm an outside starting view, yaw/pitch limits, consistent diagonal speed, and the documented 0.1 s hitch clamp. Collision is deliberately disabled; passing through solids is expected. This is not a drone dynamics model.
6. Check camera capture and release in the chosen play mode. Verify that HUD controls correspond to the actual input configuration and OS keyboard layout.
7. Before packaging, save a project-owned map and test the chosen platform's cook/package process. Run the packaged build and repeat startup/input checks. No packaging success is currently claimed.
8. Measure engine frame time and memory on a stated machine/workload if performance claims are needed. Triangle-count reduction alone does not establish a speed gain.

Record actual commands/logs, failures, fixes, screenshots and measurements. Keep all items pending until their direct evidence exists. The portable and browser validation can remain complete while this separate engine acceptance is outstanding.
