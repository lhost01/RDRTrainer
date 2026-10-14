@echo off
REM ---------------------------------------------------------------------------
REM  RDR 修改器 - one click build (Visual Studio 2022, x64)
REM ---------------------------------------------------------------------------
setlocal

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
    echo [!] vswhere.exe not found, please build the solution from Visual Studio.
    pause
    exit /b 1
)

for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe`) do set "MSBUILD=%%i"

if not defined MSBUILD (
    echo [!] MSBuild.exe not found, please build the solution from Visual Studio.
    pause
    exit /b 1
)

echo Using: %MSBUILD%
"%MSBUILD%" "%~dp0RDR修改器.sln" /p:Configuration=Release /p:Platform=x64 /m /v:minimal

if errorlevel 1 (
    echo.
    echo [!] Build failed.
    pause
    exit /b 1
)

echo.
echo [+] Build finished. The plugin is in build\Release\RDRTrainer.red
echo     (it is also copied to the game folder configured as GameDir in the vcxproj)
pause
