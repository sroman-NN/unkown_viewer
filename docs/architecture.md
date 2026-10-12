# General Architecture

This document describes the structural organization, execution model, and design principles implemented in the text viewer.

## Execution Model

The program operates on the native Win32 subsystem via the `wWinMain` entry point. It does not utilize intermediate abstraction layers, third-party wrappers, or dependent runtime environments.

Initialization follows the standard operating system flow:

1.  Registration of the window class via `RegisterClassExW`, defining a main window procedure (`MainWindowProc`), application icons, and the base canvas color.
2.  Initialization of the common controls subsystem via `InitCommonControlsEx` to ensure compatibility with Common Controls version 6.
3.  Creation of the container window with `CreateWindowExW` using the `WS_OVERLAPPEDWINDOW` and `WS_CLIPCHILDREN` flags. The latter is critical to prevent the main window background from painting over the child control area, thereby eliminating flickering.
4.  Entry into the synchronous message loop using `GetMessageW`, `TranslateMessage`, and `DispatchMessageW`.

The lifecycle concludes when `MainWindowProc` receives a `WM_DESTROY` message, invoking `PostQuitMessage` to unblock the loop and return the exit code to the operating system.

## Global Application State

All resource pointers, window handles, and viewer state variables reside in a static, contiguous memory structure named `AppState`.

The primary fields of `AppState` are:

-   `hwndMain`: Handle to the main container window.
-   `hwndEdit`: Handle to the native multiline edit control.
-   `origEditProc`: Pointer to the edit control's original window procedure, preserved to chain subclassing calls via `CallWindowProcW`.
-   `hfontEditor`: Handle to the monospaced font applied to the text being edited. - hfontstatus: handle to the font used exclusively in the bottom bar.
- hbrushwhite: solid brush for the edit control background.
- hbrushbar: solid gray brush for the status bar background.
- hpenborder: solid one-pixel pen for drawing the bar's top dividing line.
- currentfilepath: Unicode string containing the absolute path of the loaded file; if empty, the document is considered unsaved.
- statusmessage: temporary status message that overwrites the bar's secondary information.
- statustimerid: WM_TIMER identifier for expiring temporary messages.
- charwidth and charheight: dimensions in pixels of an editor font glyph, calculated once during WM_CREATE to define the caret.
- currentline and currentcol: current cursor position within the text stream, 1-based.
- ismodified: boolean flag indicating whether the in-memory buffer content differs from the state stored on disk.

This organization centralizes the state into a single data block accessible by helper functions, avoiding fragmented dynamic allocations on the heap.

## Client area division

The main window's client area is strictly divided into two functional zones:

```
+--------------------------------------------------------+
| |
| Main editing area (child edit control)                |
| - White canvas with black text                        |
| - Consolas font with solid terminal-style caret       |
| - Native vertical and horizontal scrolling            |
| - Calculated height: total_height - bar_height        |
| |
+--------------------------------------------------------+
| Top dividing line (1 pixel)                           |
| Bottom status bar (fixed height of 28 pixels)         |
| - Commands on the left, position on the right         |
+--------------------------------------------------------+
```

The physical separation between the two zones is managed within the `WM_SIZE` message handler. The child control never overlaps with the bottom bar: its height is always reduced by exactly 28 pixels relative to the total height of the client area. If the total height is less than 28 pixels, the control is resized to zero to avoid negative values ​​in the API coordinates.

## GDI Resource Management

Graphic objects (fonts, brushes, and pens) are created during the `WM_CREATE` event and remain valid for the duration of the application session.

During `WM_DESTROY` processing, each handle is explicitly released using `DeleteObject`. This protocol prevents resource leaks in the object table of the Windows kernel's GDI subsystem.