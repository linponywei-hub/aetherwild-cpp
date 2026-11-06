@echo off
chcp 65001 >nul
cd /d "%~dp0"
where cmake >nul 2>nul
if errorlevel 1 (
  echo CMake was not found. Install Visual Studio 2022 Build Tools with Desktop development with C++ and CMake.
  pause
  exit /b 1
)
cmake -S . -B build
if errorlevel 1 goto failed
cmake --build build --config Release
if errorlevel 1 goto failed
ctest --test-dir build -C Release --output-on-failure
if errorlevel 1 goto failed
if exist build\Release\aetherwild.exe (
  build\Release\aetherwild.exe
) else (
  build\aetherwild.exe
)
pause
exit /b
:failed
echo Build or tests failed. See the error above.
pause
exit /b 1
