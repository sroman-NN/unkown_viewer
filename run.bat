@echo off
setlocal

cd /d "%~dp0"

if not exist unkown_viewer.exe (
    call build.bat
    if errorlevel 1 exit /b 1
)

start "" unkown_viewer.exe %*
