# Contributing

Keep `FractalCore.h` as the single owner of geometry and virtual camera rules. The portable tools and Unreal source include that same file. Do not fork the generator or camera mathematics inside an engine actor. Maintain explicit units, bounded depth/work, finite inputs, source fingerprints, and return-on-error state preservation.

Build and run the seven portable C++ test groups, Python CLI/server tests, Node view-model tests, syntax checks, and `scripts/reproduce.py`. The local compiler helper supports clang++/g++; CMake/CTest provides the cross-platform route. A loopback bind is needed for the preview-server test. No test downloads an engine or requires paid assets.

After a reviewed source change, rebuild the exporter, then run `python3 scripts/reproduce.py --write`, inspect the generated changes, and verify again without `--write`. The wrapper rejects stale core/exporter binaries. Keep generated fixture sizes small and source hashes portable. Do not commit build caches, compiled tools, Unreal Binaries/Intermediate/Saved, credentials or personal paths.

Browser changes need real UI review: surface/wireframe, detail selection, pointer and keyboard orbit, zoom, track playback/scrubbing, completion recovery, downloads, error states and narrow screens. Preserve visible browser/engine labels. Recorded sample rate is not a renderer frame rate.

Engine changes need actual Unreal acceptance on a stated version before claiming build/render success. Follow `docs/unreal-validation.md`. A source review or portable test cannot establish UHT compilation, engine shading, input capture, packaging or profiling. Keep generated cover art distinct from actual output and never backdate work.
