# Security

The preview serves a fixed set of public files on 127.0.0.1. It rejects unknown hosts, write methods, directory listings, non-public paths and oversized assets. It has no compiler, shell, upload, telemetry, account or device-control endpoint. Browser files are local and contain no third-party scripts or analytics. This is a local development viewer, not an authenticated multi-user service.

Portable generation is bounded to depth 3, 8,000 retained cells and a 19,683-cell occupancy grid. The public CLI executes only the locally built exporter from the project's build folder and checks source fingerprints. It writes exclusive new directories and refuses overwrite. Keep build tools and source under your control; a source fingerprint is an integrity check, not a signed release or trust sandbox.

The virtual camera has no wall collision or drone physics. No aircraft, simulator autopilot, radio, sensor or other hardware connection is present. Unreal engine acceptance is pending; do not treat portable output or concept artwork as an engine result.

Use private GitHub vulnerability reporting if available. Otherwise open a general issue requesting a private reporting route. Do not post secrets or personal files. No response-time guarantee is offered.
