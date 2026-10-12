@echo off
setlocal enabledelayedexpansion

echo ===================================================
echo [Unkown Viewer] script de compilacion
echo ===================================================

cd /d "%~dp0"

where g++ >nul 2>&1
if %ERRORLEVEL% equ 0 (
    echo [Toolchain] llamando a GCC / MinGW-w64...
    
    echo [Recursos] inicio compilando resource.rc con windres...
    windres -i resource.rc -o resource.o
    if !ERRORLEVEL! neq 0 (
        echo [ERROR] fallo al compilar recursos.
        exit /b 1
    )

    echo [Build] compilando main.cpp en C++17...
    g++ -std=c++17 -O2 -mwindows -municode main.cpp resource.o -o unkown_viewer.exe -static -static-libgcc -static-libstdc++ -luser32 -lgdi32 -lcomdlg32 -lcomctl32 -lshell32 -lkernel32
    if !ERRORLEVEL! neq 0 (
        echo [ERROR] fallo en la compilacion de C++.
        exit /b 1
    )
    
    echo [OK] Compilacion completada con exito: unkown_viewer.exe
    exit /b 0
)

where cl >nul 2>&1
if %ERRORLEVEL% equ 0 (
    echo [Toolchain] Usando MSVC (cl.exe)...
    
    echo [Recursos] Compilando resource.rc con rc.exe...
    rc /nologo /fo resource.res resource.rc
    if !ERRORLEVEL! neq 0 (
        echo [ERROR] fallo al compilar recursos , chao pescao.
        exit /b 1
    )

    echo [Build] Compilando main.cpp con cl.exe...
    cl /nologo /std:c++17 /O2 /W3 /EHsc main.cpp resource.res /Fe:unkown_viewer.exe /link /SUBSYSTEM:WINDOWS user32.lib gdi32.lib comdlg32.lib comctl32.lib shell32.lib kernel32.lib
    if !ERRORLEVEL! neq 0 (
        echo [ERROR] fallo en la compilacion de C++.
        exit /b 1
    )
    
    echo [OK] compilacion completada con exito: unkown_viewer.exe
    exit /b 0
)

echo [ERROR] no se encontro ni g++ ni cl.exe en el PATH , bye bye.
exit /b 1
