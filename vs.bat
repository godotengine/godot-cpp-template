@echo off

setlocal

:: Check, if want release or debug
choice /C RD /M "Do you want to generate a release or debug project?"
if %ERRORLEVEL% equ 1 (
    set "buildType=Release"
) else (
    set "buildType=Debug"
)


if exist .sln/%buildType% (
    @REM Delete the .sln folder
    rmdir /s /q .sln/%buildType%
)

mkdir .sln
mkdir ".sln\%buildType%"
pushd ".sln\%buildType%"

cmake -G "Visual Studio 17 2022" -DCMAKE_BUILD_TYPE=%buildType% ../..
if %ERRORLEVEL% neq 0 (
    echo CMake failed.
    pause
    exit /b %ERRORLEVEL%
)

REM Create a shortcut to the .sln file
set "shortcutName=Project.sln"
set "targetPath=%~dp0.sln\%buildType%\Godot-Kafka.sln"
set "shortcutPath=%~dp0%shortcutName%.lnk"

:: Delete the shortcut if it already exists
if exist "%shortcutPath%" (
    del "%shortcutPath%"
)

REM Check if the shortcut already exists
if exist "%shortcutPath%" (
    echo Shortcut already exists.
) else (
    echo Creating shortcut...
    powershell -Command "$WshShell = New-Object -ComObject WScript.Shell; $Shortcut = $WshShell.CreateShortcut('%shortcutPath%'); $Shortcut.TargetPath = '%targetPath%'; $Shortcut.Save()"
)

start %shortcutPath%

endlocal

exit /b