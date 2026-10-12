# Compilation and Resources

This document explains the build script configuration, compiler flags, embedded resource composition, and the application manifest.

## Build Script (build.bat)

The `build.bat` file allows the project to be compiled without requiring heavy development environments or complex CMake configurations. It implements auto-detection of the toolchain available on the system.

### Detection Flow

1. Checks if `g++` (GCC / MinGW-w64) is registered in the `PATH` environment variable using `where g++`.
2. If `g++` is present, it proceeds to compile the resource file using `windres` and link using `g++`.
3. If `g++` is not found, it checks for the presence of the Visual Studio compiler (`cl.exe`) using `where cl`.
4. If `cl.exe` is active in the environment, it compiles the resources using `rc.exe` and the source code using `cl.exe`.
5. If neither tool is available, the script halts execution and returns error code 1.

### Flags for GCC / MinGW-w64

The command invoked by the script is:

```bat
windres -i resource.rc -o resource.o
g++ -std=c++17 -O2 -mwindows -municode main.cpp resource.o -o unkown_viewer.exe -static -static-libgcc -static-libstdc++ -luser32 -lgdi32 -lcomdlg32 -lcomctl32 -lshell32 -lkernel32
```

Technical explanation of each parameter:

- `-std=c++17`: Enables C++17 standard features required by the source code (advanced `constexpr`, structured bindings, type deduction). - -O2: second-level compiler optimization, applying common subexpression elimination, loop vectorization, and assembly instruction reordering.
- -mwindows: configures the PE (Portable Executable) binary subsystem as a Windows GUI application (`subsystem windows`); this prevents the operating system from creating an auxiliary black console window when the program starts.
- -municode: instructs the MinGW linker to use the `wWinMainCRTStartup` function as the C runtime library entry point, correctly recognizing the `wWinMain` function with Unicode parameters (`LPWSTR`).
- -static -static-libgcc -static-libstdc++: forces static linking of all C and C++ language components. Without these flags, the executable would require external dynamic libraries (such as `libgcc_s_seh-1.dll` or `libstdc++-6.dll`) to run on other machines; with these flags, the resulting binary is a completely self-contained file.
- -luser32 -lgdi32 -lcomdlg32 -lcomctl32 -lshell32 -lkernel32: links the operating system import libraries that resolve the Win32 API calls used by the application. ### Flags for MSVC (cl.exe)

If the user runs the script within the Visual Studio Developer Command Prompt environment:

```bat
rc /nologo /fo resource.res resource.rc
cl /nologo /std:c++17 /O2 /W3 /EHsc main.cpp resource.res /Fe:unkown_viewer.exe /link /SUBSYSTEM:WINDOWS user32.lib gdi32.lib comdlg32.lib comctl32.lib shell32.lib kernel32.lib
```

- /std:c++17: C++17 mode in the MSVC frontend.
- /O2: Optimization for maximum execution speed.
- /W3: Warning level 3 for strict type checking.
- /EHsc: Standard synchronous C++ exception model.
- /SUBSYSTEM:WINDOWS: Defines the graphical subsystem in the Microsoft linker (link.exe).

## Resource file (resource.rc)

The resource.rc file defines the inclusion of non-executable binary components in the .rsrc section of the PE executable:

```rc
#include <windows.h>
#include "resource.h"

IDI_APP_ICON ICON "app.ico"
1 RT_MANIFEST "app.manifest"

VS_VERSION_INFO VERSIONINFO
FILEVERSION     1,0,0,0
PRODUCTVERSION  1,0,0,0
FILEFLAGSMASK   VS_FFI_FILEFLAGSMASK
FILEFLAGS       0
FILEOS          VOS_NT_WINDOWS32
FILETYPE        VFT_APP
FILESUBTYPE     VFT2_UNKNOWN
BEGIN
    BLOCK "StringFileInfo"
    BEGIN
        BLOCK "040904b0"
        BEGIN
            VALUE "CompanyName",      "NTASH"
            VALUE "FileDescription",  "Unkown Viewer"
            VALUE "FileVersion",      "1.0.0.0"
            VALUE "InternalName",     "unkown_viewer"
            VALUE "OriginalFilename", "unkown_viewer.exe"
            VALUE "ProductName",      "Unkown Viewer"
            VALUE "ProductVersion",   "1.0.0.0"
        END
    END
    BLOCK "VarFileInfo"
    BEGIN
        VALUE "Translation", 0x0409, 1200
    END
END
```

Included components:

1. idi_app_icon (ID 101 defined in resource.h): reference to the physical file app.ico. This icon is loaded for the process representation in Windows Explorer as well as for the title bar (hIcon) and taskbar (hIconSm) icons.
2. 1 rt_manifest: embeds the XML manifest file within the binary, eliminating the need for external .manifest files on disk.
3. vs_version_info: a standard Windows metadata structure that allows Explorer and diagnostic tools to query the product version, internal name, and copyright information.

## Application manifest (app.manifest)

The app.manifest file is a structured XML document that declares platform capabilities required by the application before the kernel begins execution:

### Enabling Common Controls v6

Declares a dependency on version 6.0.0.0 of microsoft.windows.common-controls:

```xml
<dependency>
  <dependentAssembly>
    <assemblyIdentity
        type="win32"
        name="Microsoft.Windows.Common-Controls"
        version="6.0.0.0"
        processorArchitecture="*"
        publicKeyToken="6595b64144ccf1df"
        language="*"/>
  </dependentAssembly>
</dependency>
```

This directive enables modern Windows visual themes for common controls. Without it, the edit control's scrollbars would be rendered using the classic Windows 95 visual style.

### Per-monitor v2 support for high resolution (DPI)

Declares compatibility with high-pixel-density displays (4K, Retina laptop screens):

```xml
<application xmlns="urn:schemas-microsoft-com:asm.v3">
  <windowsSettings>
    <dpiAware xmlns="http://schemas.microsoft.com/SMI/2005/WindowsSettings">true/pm</dpiAware>
    <dpiAwareness xmlns="http://schemas.microsoft.com/SMI/2016/WindowsSettings">PerMonitorV2, PerMonitor</dpiAwareness>
  </windowsSettings>
</application>
```

With PerMonitorV2, Windows does not scale the window by artificially stretching bitmaps (which would cause blurry text); instead, it allows the window procedure to receive scaling messages and calculate font heights and coordinates based on the actual physical pixels of each monitor.

## Execution script (run.bat)

The run.bat file automates the daily testing and execution cycle:

```bat
@echo off
setlocal

cd /d "%~dp0"

if not exist unkown_viewer.exe (
    echo [Unkown Viewer] Ejecutable no encontrado. Compilando primero...
    call build.bat
    if errorlevel 1 exit /b 1
)

echo [Unkown Viewer] Lanzando unkown_viewer.exe...
start "" unkown_viewer.exe %*
```

Its behavior is as follows:

1. Sets the working directory to the folder where the script resides (%~dp0).
2. Checks if the executable `unkown_viewer.exe` exists on disk.
3. If it does not exist, it immediately invokes `build.bat` to generate the binary; if compilation fails, it halts execution with exit code 1.
4. If the executable is already present (or was successfully generated), it runs it in the background using the `start` command, passing along any arguments received via the command line (%*).
