@echo off
echo Building Visual Studio project files...

cd build

set TOOLSET_ARG=
set VSWHERE="%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"

if not exist %VSWHERE% (
    echo [WARNING] vswhere.exe not found! Falling back to default toolset.
    goto :run_premake
)

:: Get the latest Visual Studio installation version (e.g., "18.0.x", "17.10.x")
for /f "usebackq tokens=*" %%i in (`%VSWHERE% -latest -property installationVersion`) do (
    set VS_VERSION=%%i
)

if "%VS_VERSION%"=="" (
    echo [WARNING] Could not detect Visual Studio version! Falling back to default toolset.
    goto :run_premake
)

set VS_MAJOR=%VS_VERSION:~0,2%
echo [INFO] Detected latest installed Visual Studio Major Version: %VS_MAJOR%

:: Map Visual Studio major versions to their default Platform Toolsets
if "%VS_MAJOR%"=="18" set TOOLSET_ARG=--toolset_ver=v145
if "%VS_MAJOR%"=="17" set TOOLSET_ARG=--toolset_ver=v143
if "%VS_MAJOR%"=="16" set TOOLSET_ARG=--toolset_ver=v142
if "%VS_MAJOR%"=="15" set TOOLSET_ARG=--toolset_ver=v141

if not "%TOOLSET_ARG%"=="" (
    echo [INFO] Automatically configuring toolset corresponding to VS version %VS_MAJOR%...
) else (
    echo [WARNING] Unknown Visual Studio version: %VS_MAJOR%. Using default...
)

:run_premake
echo.
echo Running Premake5...
premake5.exe vs2022 %TOOLSET_ARG%

if %errorlevel% neq 0 (
    echo Error running Premake5!
    pause
    exit /b %errorlevel%
)

echo Project files generated successfully!
echo Opening solution...

cd ..

echo Looking for solution file...
for %%f in (*.sln) do (
    echo Found solution file: %%f
    echo Waiting 5 seconds before opening solution...
    timeout /t 5 /nobreak >nul
    echo Opening solution file...
    start "%%f" "%%f"
    goto :found
)

echo Solution file not found!
pause
goto :end

:found
echo Solution opened successfully!

:end
