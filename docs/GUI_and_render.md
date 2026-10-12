# GDI interface and rendering

This document details the graphics composition mechanics, the text editing control, typographic metric calculations, and the rendering of the bottom bar.

## Multiline edit control

The editing area does not require custom drawing engines or software rendering libraries. It relies on the operating system's standard "Edit" control, instantiated via `CreateWindowExW` using the predefined "EDIT" class.

The applied style flags are:

- `WS_CHILD`: Specifies that the window is a child of the main container window.
- `WS_VISIBLE`: Makes the control visible upon creation.
- `WS_VSCROLL`: Enables the native vertical scrollbar.
- `WS_HSCROLL`: Enables the native horizontal scrollbar.
- `ES_MULTILINE`: Allows multiple lines of text.
- `ES_AUTOVSCROLL`: Automatically scrolls text vertically when the cursor moves beyond the client area boundaries.
- `ES_AUTOHSCROLL`: Scrolls text horizontally when a line exceeds the visible width.
- `ES_WANTRETURN`: Forces the Enter key to insert a line break (`\r\n`) instead of sending the default dialog message.
- `ES_NOHIDESEL`: Keeps the active text selection visible even when the control loses keyboard focus.

After the control is created, the `EM_SETMARGINS` message is sent with the `EC_LEFTMARGIN` and `EC_RIGHTMARGIN` flags, setting a ten-pixel margin on the left and right sides. This prevents glyphs from touching the window edges and improves readability.

## Font metrics and terminal-style caret

The viewer uses the Consolas font as the primary typeface for code and text. During the `WM_CREATE` message, the window's device context (HDC) is queried using `GetDeviceCaps` with `LOGPIXELSY` to calculate the font height in points, independent of physical resolution:

```cpp
int fontHeight = -MulDiv(12, GetDeviceCaps(hdc, LOGPIXELSY), 72);
```

The font is instantiated using `CreateFontW`, requesting `CLEARTYPE_QUALITY` and `FIXED_PITCH | FF_MODERN`. Once the font is selected into the temporary HDC, `GetTextMetricsW` is called on a `TEXTMETRICW` structure to extract:

- `tmAveCharWidth`: average character width in pixels (`charWidth`).
- `tmHeight`: glyph height in pixels (`charHeight`).

Using these dimensions, the viewer implements a solid, terminal-style caret. Instead of the standard thin vertical Windows line (a one-pixel I-beam), the caret is created by calling:

```cpp
CreateCaret(hwndEdit, nullptr, charWidth, charHeight);
ShowCaret(hwndEdit);
```

This draws a rectangular block covering the entire area of ​​the current character. Since the edit control manages internal caret positioning via `SetCaretPos`, the block automatically relocates when typing, deleting, or navigating with arrow keys.

## Edit Control Coloring

Standard Windows controls do not paint their own backgrounds when a specific palette is required. They delegate color selection to the parent window via the `WM_CTLCOLOREDIT` message (when the control is interactive) and `WM_CTLCOLORSTATIC` (when in read-only mode).

In the `MainWindowProc` procedure, these messages are intercepted and the following is executed:

1. `SetTextColor(hdc, color_canvas_txt)`: sets the text color to a matte black (RGB 15, 15, 15). 2. setbkcolor(hdc, color_canvas_bg): sets the background color to pure white (RGB 255, 255, 255).
3. The handle to the solid white brush (hbrushwhite), previously allocated, is returned. The operating system uses this brush to clear and fill areas not occupied by characters.

## Rendering the bottom status bar

The bottom command and status bar is not a secondary Windows control, but rather a reserved area drawn directly within the container window's WM_PAINT handler.

The painting cycle follows this order:

1. Call `BeginPaint` to obtain the device context (`hdc`) and fill the `PAINTSTRUCT` structure.
2. Retrieve client area dimensions using `GetClientRect`.
3. Define the bar's rectangle:
- `left` = 0
- `top` = `client.bottom` - 28
- `right` = `client.right`
- `bottom` = `client.bottom`
4. Fill the rectangle using `FillRect` with a charcoal gray brush (RGB 38, 38, 42).
5. Draw a horizontal line along the top edge of the rectangle (`y` = `top`) using a light gray pen (RGB 55, 55, 60)—via `MoveToEx` and `LineTo`—to mark the separation from the white canvas above.
6. Set the text background mode to transparent using `SetBkMode` to avoid colored boxes around the characters.
7. Select the `hFontStatus` font (Segoe UI Bold).
8. Draw the command block on the left using `DrawTextW` with `DT_LEFT | DT_VCENTER | DT_SINGLELINE`. The text displays the active commands:
`CTRL+O (OPEN)   CTRL+S (SAVE)   CTRL+X (CLOSE)   CTRL+Z (UNDO)   CTRL+A (ALL)`
9. Format and draw the status block on the right using `DrawTextW` with `DT_RIGHT | DT_VCENTER | DT_SINGLELINE`. It displays the dynamic cursor position (`Ln X, Col Y | UTF-8`) or the active temporary message (in light blue, RGB 130, 185, 250).
10. Restore the objects previously selected into the device context and call `EndPaint`.

## Preventing flickering and clipping

To prevent flickering during resizing:

- The window class uses `CS_HREDRAW | CS_VREDRAW`. - The main window has the WS_CLIPCHILDREN flag set, preventing the parent window's background erasure from painting over the area occupied by the text control.
- When the cursor position changes or a status message is triggered, only the rectangle corresponding to the bottom bar is invalidated—by calling InvalidateRect with rcBar and bErase = false—without forcing a repaint of the edit control.
