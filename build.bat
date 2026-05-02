@echo off
cd /d "%~dp0build"
cmake .. -G "MinGW Makefiles" && cmake --build .
if %errorlevel% equ 0 (
    echo Build success
) else (
    echo Build failed
)
pause