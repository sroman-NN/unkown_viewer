@echo off
setlocal enabledelayedexpansion

echo ===================================================
echo [Unkown Viewer] build script
echo ===================================================

cd /d "%~dp0"

where g++ >nul 2>&1
if %ERRORLEVEL% equ 0 (
    echo [Toolchain] calling to  GCC / MinGW-w64...
    
    echo [Recursos] start... compiling process started for resource.rc with windres...
    windres -i resource.rc -o resource.o
    if !ERRORLEVEL! neq 0 (
        echo [ERROR] faild to compile resources.
        exit /b 1
    )

    echo [Build] cinpiling main.cpp in C++17...
    g++ -std=c++17 -O2 -mwindows -municode main.cpp resource.o -o unkown_viewer.exe -static -static-libgcc -static-libstdc++ -luser32 -lgdi32 -lcomdlg32 -lcomctl32 -lshell32 -lkernel32
    if !ERRORLEVEL! neq 0 (
        echo [ERROR] faild to compile C++.
        exit /b 1
    )
    
    echo [OK] build completed sucessfully: unkown_viewer.exe
    exit /b 0
)

where cl >nul 2>&1
if %ERRORLEVEL% equ 0 (
    echo [Toolchain] using MSVC (cl.exe)...
    
    echo [Recursos] compiling resource.rc with rc.exe...
    rc /nologo /fo resource.res resource.rc
    if !ERRORLEVEL! neq 0 (
        echo [ERROR] faild to compile resources , chao pescao.
        exit /b 1
    )

    echo [Build] compiling main.cpp with cl.exe...
    cl /nologo /std:c++17 /O2 /W3 /EHsc main.cpp resource.res /Fe:unkown_viewer.exe /link /SUBSYSTEM:WINDOWS user32.lib gdi32.lib comdlg32.lib comctl32.lib shell32.lib kernel32.lib
    if !ERRORLEVEL! neq 0 (
        echo [ERROR] faild to compile C++.
        exit /b 1
    )
    
    echo [OK] build completed sucessfully: unkown_viewer.exe
    exit /b 0
)

echo [ERROR]   g++ or cl.exe not found in path 
exit /b 1
