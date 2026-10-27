# Construction, coordinate conventions, and evidence

## Finite construction

The implementation defines a Menger-style voxel rule. At each base-three digit level, remove a cell if two or more of its x/y/z digits are the centre digit 1. One step retains 20 of 27 sub-cells. Depth `n` therefore retains `20^n` cells in a `3^n` grid per axis. Depth is limited to 0–3: 8,000 retained cells in a 19,683-cell occupancy grid at the maximum.

The mesh is centred at the origin. The generator visits occupied cells deterministically and emits only faces next to an empty or outside neighbour. Each exposed quad has four vertices, per-face normals/UVs and two triangles. This keeps actual surface data and avoids drawing hidden cube interfaces. Vertices are duplicated per face for flat shading; there is no welding, topology certification or fabrication guarantee.

An independent face-count recurrence is `F(n) = 20 F(n−1) − 48 × 8^(n−1)`, with `F(0)=6`: the assembly has 24 touching sub-cell pairs, each hiding two interfaces with `8^(n−1)` boundary tiles. Thus `F(n) = 2 × 20^n + 4 × 8^n`, giving 6, 72, 1,056 and 18,048 faces. Tests check those counts independently of the neighbour loop.

For side `S` metres, cell side is `S / 3^n`. Voxel volume is `20^n × cellSide^3`; exposed area is `F(n) × cellSide^2`. Tests also integrate signed volume and triangle areas from actual vertex/index data, checking those independent values against the voxel/face model. Bounds, finite coordinates, attributes and index ranges are checked. The generator has no random seed or trained model.

The baseline is `12 × retainedCells` triangles: six complete quad faces per retained cube. The exposed-face triangle count is compared with that arithmetic baseline. This is not a renderer benchmark, draw-call measurement, optimality proof or engine performance result.

## Coordinates and triangle order

Core positions use metres with X forward, Y right and Z up for camera semantics. Core triangle indices align `cross(B−A, C−A)` with outward unit normals. The browser keeps those numeric coordinates and uses a left-handed forward-positive view/projection. It explicitly selects clockwise screen front faces with `gl.frontFace(gl.CW)`; a test projects a real outside cube face and checks its signed screen area. See the [Khronos WebGL specification](https://registry.khronos.org/webgl/specs/latest/1.0/) for front-face state.

Epic documents Unreal's [coordinate system](https://dev.epicgames.com/documentation/unreal-engine/coordinate-system-and-spaces-in-unreal-engine) and [default centimetre units](https://dev.epicgames.com/documentation/en-us/unreal-engine/units-of-measurement-in-unreal-engine?application_version=5.4). The actor/pawn multiply core positions by 100. Normals and UVs remain unscaled. Keep actor scale at 1; edit side length through the metre property.

Rather than guess the engine's index convention, the actor samples the first triangle/normal from Epic's [GenerateBoxMesh](https://dev.epicgames.com/documentation/unreal-engine/API/Plugins/ProceduralMeshComponent/UKismetProceduralMeshLibrary/GenerateBoxMesh) once. It chooses the same cross/normal sign using the tested optional triangle reversal. Malformed or degenerate native samples clear the mesh and report an error. Vertex arrays, normals and UVs are passed to [CreateMeshSection_LinearColor](https://dev.epicgames.com/documentation/unreal-engine/API/Plugins/ProceduralMeshComponent/UProceduralMeshComponent/CreateMeshSectio-) with collision disabled. This is reviewed source logic; it has not run in an engine here.

## Virtual camera

Position is in metres, look angles in degrees, speed in m/s, and timestep in seconds. Each finite step is 0–0.1 s, with controls in [-1,1]. Yaw/pitch keyboard rates are 90 degrees/s at full input. Mouse look is a bounded angular delta. Yaw wraps and pitch clamps to ±89°. Position is bounded to ±10,000 m per axis.

Forward follows yaw/pitch, right follows yaw, and vertical follows world Z. The combined world-space movement vector is normalised if its length exceeds one, including when pitch and world-vertical controls combine. Invalid requests leave core state unchanged. Constant-orientation translation scales with timestep; turning uses explicit per-step integration and is not claimed to be an exact continuous path. The Unreal wrapper clamps frame hitches to 0.1 s and drops excess time.

The recorded demo is a straight 60 m path at 5 m/s, sampled 721 times over 12 s. Its samples are actual outputs of `step`, not manually invented poses. Browser interpolation only displays those poses. There is no wall collision, inertia, vehicle dynamics, autopilot, radio or real flight. A trajectory can pass through solids.

## Portable build and reproduction

The same header is included by the Unreal module and the portable C++ tests/exporter. The exporter records SHA-256 fingerprints of its compiled core and producer source. The Python wrapper compares them with current source before executing export commands, then writes exclusive new-directory bundles. It does not accept arbitrary external tool paths. CMake watches those inputs to refresh the fingerprint definitions on reconfiguration.

Native stdout uses binary mode on Windows. JSON/OBJ scalars are rounded to six fractional decimal places, and near-zero values are normalised to avoid signed-zero platform differences. Geometric tests use full-precision core values. This export precision is for portable visualisation, not manufacturing tolerance.

`scripts/reproduce.py` runs actual CLI bundles, generates the orthographic projection from exported faces, and compares nine committed artifacts byte-for-byte, including source/output hashes. The projection uses `math.fsum` and sorts depth to nine decimal places, below its three-decimal drawing precision. This keeps equal-depth faces in stable mesh order across Python versions. The projection is a portable mesh drawing, not an Unreal render. Local clang++ strict-warning tests and address/undefined-behaviour sanitizer runs passed. CMake/CTest and other OS/compiler results are established by their actual CI runs, separately from local checks.

## Unreal evidence boundary

Default startup source uses Epic's [SpawnActor API](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/UWorld/SpawnActor) to create the mesh and lights, with the pawn/HUD selected by the game mode. The project references the built-in Entry map and material. Source, module/targets and controls are supplied; no binary engine content is copied.

No Unreal Editor/Build Tool is installed here. Compilation/UHT, startup, native winding, lighting, input capture, rendering, cooking, packaging and profiling remain unverified. Follow [the explicit engine checklist](unreal-validation.md) on an existing licensed installation. Portable tests and a working browser preview cannot close those engine checks.
