# unkown viewer technical documentation

This directory contains the complete technical specifications for the unkown viewer application, developed in native C++17 using the Win32 API.

## Document Index

The documentation is divided into five technical modules:

1. [architecture.md](/docs/architecture.md):
Describes the native execution model, the `WinMain` entry point, the global `AppState` structure, the subdivision of the client area into the canvas and the command bar, and the GDI resource management policy.

2. [GUI_and_render.md](/docs/GUI_and_render.md):
Details the creation and configuration of the multiline text control, the calculation of font metrics for the solid terminal-style caret, the coloring protocol via `WM_CTLCOLOREDIT`, and the direct GDI rendering of the bottom bar within `WM_PAINT`.

3. [events_and_subclassing.md](/docs/events_and_subclassing.md):
Explains the subclassing technique applied to the edit control's window procedure, the capture of keyboard shortcuts (Ctrl+O, Ctrl+S, Ctrl+X, Ctrl+Z, Ctrl+Y, Ctrl+A), real-time line and column calculation, notification timer handling, and file reception via `WM_DROPFILES`. 

4. [storage_and_formats.md](/docs/storage_and_formats.md):
documents physical file reading and writing using kernel32 subsystem functions, the 128 MB memory safety limit, UTF-8 BOM prefix detection and removal, bidirectional conversion between disk UTF-8 sequences and in-memory UTF-16 strings, and comdlg32 dialog parameters.

5. [compilation_and_rcs.md](/docs/compilation_and_rcs.md):
describes the compiler auto-detection logic in build.bat (GCC and MSVC), optimization and full static linking flags, resource definitions in resource.rc, and the integration of app.manifest for Common Controls v6 and Per-Monitor DPI v2 support.

## repository file summary

the project root contains the following files:

- main.cpp: monolithic application source code. contains the main procedure, the subclass procedure, text conversion routines, and direct Win32 calls.
- resource.h: C-language header defining numeric identifiers for compiled resources (IDI_APP_ICON).
- resource.rc: Windows resource script that embeds the application icon, manifest, and version information into the executable.
- app.ico: binary icon file with 16x16 and 32x32 pixel resolutions at 32-bit depth with an alpha channel. - app.manifest: XML manifest declaring Windows platform dependencies (common-controls 6.0) and per-monitor v2 scaling.
- build.bat: Batch script that detects g++ or cl.exe, compiles resources using windres or rc, and generates the unkown_viewer.exe binary with full static linking.
- run.bat: Batch script that checks for the executable's existence, invokes build.bat if necessary, and launches the process.