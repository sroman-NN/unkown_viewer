# storage and encoding

This document describes disk file handling, the character encoding transformation flow, and system interaction mechanisms.

## reading files using the native API

File loading bypasses high-level input/output streams (such as `fstream`) to operate directly with Windows kernel functions (`kernel32.dll`), ensuring direct memory transfers.

The `loadfilecontent` function implements the following procedure:

1. Opening the file handle:
```cpp
HANDLE hFile = CreateFileW(
path.c_str(),
GENERIC_READ,
FILE_SHARE_READ,
nullptr,
OPEN_EXISTING,
FILE_ATTRIBUTE_NORMAL,
nullptr
); 
```
`FILE_SHARE_READ` is used to avoid blocking concurrent access by other processes wishing to read the file simultaneously.
2. Measuring size in bytes:
The `LARGE_INTEGER` structure is obtained via `GetFileSizeEx`. A strict security check is applied, limiting the read operation to 128 megabytes (134,217,728 bytes). This restriction protects the process from exhausting memory or crashing the Win32 edit control, which was not designed to handle gigabytes of text in a single contiguous buffer.
3. Synchronous reading:
A contiguous byte buffer (`std::string`) is allocated based on the exact reported size. `ReadFile` is executed, passing the buffer's starting pointer and the required number of bytes. Upon completion, the handle is immediately closed using `CloseHandle`.

## handling encodings and BOM

Text files on modern platforms are typically encoded in UTF-8, whereas the graphics subsystem and native Windows controls operate internally using UTF-16 (`wchar_t`). ### Byte Order Mark (BOM) detection

Before converting the received binary sequence into readable text, the viewer analyzes the first three bytes of the buffer:

- byte 0: 0xef
- byte 1: 0xbb
- byte 2: 0xbf

If these three bytes match the standard UTF-8 BOM signature, they are discarded using `substr(3)`. Otherwise, they would appear in the editor as null characters or invalid glyphs.

### Conversion to UTF-16

The utility function `utf8towide` performs a two-phase conversion process using the `MultiByteToWideChar` API:

1. First call, passing a null pointer for the output buffer to query the exact number of `wchar_t` characters required for the `CP_UTF8` code page:
```cpp
int req = MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), nullptr, 0); 
```
2. If the initial conversion fails (for example, if the file is in legacy ANSI/Windows-1252 format and contains byte sequences that violate the strict UTF-8 specification), the function automatically falls back to the active system character set using `CP_ACP`. This ensures the viewer opens any readable file without failing due to obscure errors.
3. Allocation of an `std::wstring` of length `req` and execution of the second `MultiByteToWideChar` call to write the glyphs into the final buffer.

Once the text is obtained in UTF-16 format, it is injected into the control using `SetWindowTextW`, and `SendMessageW(hwndEdit, EM_EMPTYUNDOBUFFER, 0, 0)` is called so that the initial loaded state does not count as a reversible undo operation. ## saving files to disk

The `savefilecontent` function performs the inverse operation symmetrically:

1. Queries the length of the text stored in the control using `GetWindowTextLengthW`.
2. Allocates a `wchar_t` buffer of size `length + 1` and extracts the full content by calling `GetWindowTextW`.
3. Converts the UTF-16 string into a UTF-8 byte sequence using `WideToUtf8`, which relies on `WideCharToMultiByte` with the `CP_UTF8` flag.
4. Creates or truncates the file on disk using `CreateFileW` with `GENERIC_WRITE` and `CREATE_ALWAYS`.
5. Writes the bytes from the resulting UTF-8 buffer using `WriteFile`. If the operation succeeds, it closes the handle using `CloseHandle`.

The application intentionally saves the file in "clean" UTF-8 without a BOM prefix, adhering to software development industry standards to avoid incompatibilities with lexical analyzers, compilers, and version control systems.

## standard system dialogs

File system interaction for opening or saving relies on dialogs provided by `comdlg32.dll`:

- `GetOpenFileNameW`: displays the standard "Open" dialog.
- `GetSaveFileNameW`: displays the standard "Save" dialog.

Both functions are configured using the `OPENFILENAMEW` structure with the following critical flags:

- ofn_pathmustexist: requires the user to navigate to a path that physically exists.
- ofn_filemustexist (when opening): prevents the user from attempting to open a non-existent file.
- ofn_overwriteprompt (when saving): automatically displays a confirmation prompt if the selected name already exists in the destination directory.
- ofn_nochangedir: a crucial flag in Win32 environments; it prevents the dialog from altering the process's current working directory. Without this flag, user navigation within the explorer would change the executable's base path, breaking relative paths in subordinate tools or associated scripts.

## Confirmation of pending changes

When an attempt is made to close the application (via the Ctrl+X shortcut, the title bar's close button, or the system menu), the `MainWindowProc` procedure intercepts the `WM_CLOSE` message.

If the `isModified` variable is true:

- A modal message box is displayed using `MessageBoxW` with the flags `MB_YESNOCANCEL | MB_ICONQUESTION`.
- If the user clicks "Yes" (`IDYES`), `SaveFileDialog(false)` is invoked to commit changes to disk before destroying the window.
- If the user clicks "Cancel" (`IDCANCEL`), `WM_CLOSE` processing is aborted, and execution continues without closing the application.
- If the user clicks "No" (`IDNO`), the process proceeds directly to `DestroyWindow(hwnd)`, discarding the in-memory modifications.
