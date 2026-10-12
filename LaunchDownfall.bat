@echo off
setlocal EnableExtensions
rem Close Unreal Editor before running so the module DLL can be rebuilt.
set "DOWNFALL_ENGINE=C:\Program Files\Epic Games\UE_5.8"
if defined UE_ROOT set "DOWNFALL_ENGINE=%UE_ROOT%"
if not "%~1"=="" set "DOWNFALL_ENGINE=%~1"
if not exist "%DOWNFALL_ENGINE%\Engine\Build\BatchFiles\Build.bat" (
    echo Unreal Engine 5.8 was not found. Pass its folder to this script or set UE_ROOT.
    pause
    exit /b 1
)
pushd "%~dp0"
echo Building DownfallEditor. Please keep Unreal Editor closed until this finishes.
call "%DOWNFALL_ENGINE%\Engine\Build\BatchFiles\Build.bat" DownfallEditor Win64 Development -Project="%~dp0Downfall.uproject" -WaitMutex -NoHotReloadFromIDE
if errorlevel 1 (
    echo Build failed. Copy the first compiler error above.
    popd
    pause
    exit /b 1
)
echo Opening Downfall. First launch generates Breakwater and compiles its materials.
start "" "%DOWNFALL_ENGINE%\Engine\Binaries\Win64\UnrealEditor.exe" "%~dp0Downfall.uproject"
popd
endlocal
