# Events and Subclassing

This document details system message interception, the text control subclassing mechanism, keyboard shortcut processing, and file drag-and-drop integration.

## Window Subclassing Mechanism

The native edit control has an internal window procedure implemented by the Windows `user32` subsystem. To alter its behavior (such as intercepting shortcuts and managing a custom cursor), the viewer employs the Win32 window subclassing technique.

During initialization within `WM_CREATE`, the following executes:

```cpp
g_app.origeditproc = (WNDPROC)SetWindowLongPtrW(
g_app.hwndEdit,
GWLP_WNDPROC,
(LONG_PTR)SubclassedEditProc
);
```

This replaces the active procedure address with `SubclassedEditProc` and stores the original function in `origeditproc`. Any message not explicitly consumed by the custom logic is forwarded to `CallWindowProcW` using the original pointer, ensuring that standard selection, scrolling, and editing behaviors remain intact.

## Focus and Caret Lifecycle

Windows automatically destroys window carets when a control loses keyboard focus. To preserve the solid block-style cursor:

1. Upon receiving `WM_SETFOCUS`:
- Control is first delegated to the original procedure via `CallWindowProcW`. 
- `SetupEditorCaret()` executes: `CreateCaret` is invoked with `charWidth` and `charHeight` dimensions calculated from the Consolas font, and the caret is made visible via `ShowCaret`. 
- `UpdateCursorPosition()` is called to immediately synchronize the caret's coordinates.
2. Upon receiving `WM_KILLFOCUS`:
- The cursor's graphical resource is destroyed via `DestroyCaret()` to prevent orphaned HDC artifacts. 
- Control is passed to `CallWindowProcW`. ## Keyboard shortcut processing

When the user presses a key within the edit control, the `WM_KEYDOWN` message is evaluated in `SubclassedEditProc`.

To detect key combinations, the state of the Control key is checked against the virtual key map:

```cpp
bool isCtrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
```

If the flag is active, the `wParam` parameter is examined:

- 'O' or 'o' key: Opens the file selection dialog using `OpenFileDialog()`. Returns zero immediately to prevent the control from interpreting the key as text input or emitting a non-printable control character.
- 'S' or 's' key: Invokes `SaveFileDialog(false)` to save changes to the active file or prompt for a path if the document is new. Returns zero.
- 'X' or 'x' key: Sends the `WM_CLOSE` message to the main window via `PostMessageW(g_app.hwndMain, WM_CLOSE, 0, 0)`, initiating the exit confirmation protocol. Returns zero.
- 'A' or 'a' key: Selects the entire buffer content by sending `EM_SETSEL` with a start of 0 and an end of -1 to the control. Updates the bar coordinates and returns zero.
- 'Z' or 'z' key: Evaluates the Shift key state. If Shift is pressed (Ctrl+Shift+Z), it invokes redo via `g_app.undoMgr.PerformRedo()`. If Shift is not pressed (Ctrl+Z), it invokes multi-level undo via `g_app.undoMgr.PerformUndo()`. Updates coordinates, the modification state relative to the on-disk content, and the status bar. Returns zero.
- 'Y' or 'y' key: Invokes multi-level redo via `g_app.undoMgr.PerformRedo()`. Updates coordinates, the modification flag, and the visual state. Returns zero. If the pressed key does not correspond to any shortcut involving the Control key, the conditional block is exited, and the message proceeds normally to CallWindowProcW.

## Line and column calculation

To reflect the cursor position in real time, the subclass procedure monitors the following events:

- wm_keyup: completion of keyboard-based movement or character insertion.
- wm_lbuttonup: cursor repositioning via mouse click.
- wm_mousemove: continuous updates during mouse-drag selection operations.

Additionally, the main window listens for the EN_CHANGE notification code within WM_COMMAND, which is triggered whenever the text is modified.

The `updatecursorposition` function performs the calculation using three successive Win32 messages:

1. Selection query using EM_GETSEL:
```cpp
DWORD selstart = 0; 
SendMessageW(g_app.hwndEdit, EM_GETSEL, (WPARAM)&selstart, 0); 
```
Obtains the absolute character index of the insertion point.
2. Line determination using EM_LINEFROMCHAR:
```cpp
LRESULT line = SendMessageW(g_app.hwndEdit, EM_LINEFROMCHAR, (WPARAM)selstart, 0); 
```
Converts the character index into the line number (zero-based).
3. Column calculation using EM_LINEINDEX:
```cpp
LRESULT lineIndex = SendMessageW(g_app.hwndEdit, EM_LINEINDEX, (WPARAM)line, 0); 
LRESULT col = (LRESULT)selstart - lineIndex; 
```
EM_LINEINDEX returns the index where the current line begins. Subtracting the start of the line from the current cursor position yields the exact column index.

The resulting values ​​are incremented by one (to provide a one-based index for the user), and invalidation is requested only for the `rcBar` rectangle using `InvalidateRect`.

## Timer and status messages

When an action such as saving or opening a file occurs, the `SetStatusTempMessage` function is activated:

- It assigns a descriptive string to `statusMessage`. - If a previous timer exists, it is cancelled using `KillTimer`.
- A new timer is started by calling `SetTimer(hwndMain, 1, durationMs, nullptr)` with a typical duration of 3000 milliseconds.
- The bar is forced to redraw.

When the time expires, the operating system sends `WM_TIMER` with `wParam` set to 1. The handler cancels the timer, clears the `statusMessage` string, and redraws the bar, restoring the normal coordinate display.

## Drag-and-drop support

After initializing the window, `DragAcceptFiles(hwnd, true)` is called to enable integration with the Windows shell.

When the user drops one or more files onto the viewer window:

1. The main window receives the `WM_DROPFILES` message.
2. The `wParam` parameter is cast to an `HDROP` handle.
3. `DragQueryFileW(hdrop, 0, szFile, MAX_PATH)` is called to extract the absolute path of the first file involved in the operation.
4. `LoadFileContent` is invoked with that path. If the read is successful, the active path is updated, the modification flag is cleared, and the window title is refreshed.
5. `DragFinish(hdrop)` is called to free the memory allocated by Windows Explorer for the drag structure.