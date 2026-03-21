@echo off
setlocal

set "SCRIPT_DIR=%~dp0"
set "BASH_EXE="

if exist "C:\Program Files\Git\bin\bash.exe" set "BASH_EXE=C:\Program Files\Git\bin\bash.exe"
if not defined BASH_EXE if exist "C:\Program Files\Git\usr\bin\bash.exe" set "BASH_EXE=C:\Program Files\Git\usr\bin\bash.exe"

if not defined BASH_EXE (
    for /f "delims=" %%I in ('where bash.exe 2^>nul') do (
        set "BASH_EXE=%%I"
        goto found_bash
    )
)

:found_bash
if not defined BASH_EXE (
    echo Git Bash not found. Install Git for Windows or add bash.exe to PATH. 1>&2
    exit /b 1
)

"%BASH_EXE%" -lc "cd \"$(cygpath '%SCRIPT_DIR%')\" && ./mfx %*"
exit /b %ERRORLEVEL%
