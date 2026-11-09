@echo off
chcp 65001 >nul
cd /d "%~dp0"
where cmake >nul 2>nul
if errorlevel 1 (
 echo Install Visual Studio 2022 Build Tools with Desktop development with C++ and CMake first.
 pause
 exit /b 1
)
cmake -S . -B build
if errorlevel 1 goto failed
cmake --build build --config Release
if errorlevel 1 goto failed
ctest --test-dir build -C Release --output-on-failure
if errorlevel 1 goto failed
start "" /B powershell -NoProfile -Command "Start-Sleep -Seconds 2; Start-Process 'http://localhost:4173/'"
if exist build\Release\aetherwild_workstation.exe (
 build\Release\aetherwild_workstation.exe --root "%CD%"
) else (
 build\aetherwild_workstation.exe --root "%CD%"
)
pause
exit /b
:failed
echo Build or tests failed. See errors above.
pause
exit /b 1
