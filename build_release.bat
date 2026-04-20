@echo off
echo =========================================
echo Configuring CMake for Release Build...
echo =========================================
cmake -B build -S .

echo.
echo =========================================
echo Building Release Executable...
echo =========================================
cmake --build build --config Release

echo.
echo Done! If successful, the executable is located in the build\Release folder.
pause
