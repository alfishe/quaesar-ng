@echo off
pushd %~dp0
setlocal

set BUILD_DIR=build
set ROOT_DIR=%cd%\..\

rem # Check if the current folder is named "scripts"
for %%A in ("%cd%") do set CURRENT_FOLDER=%%~nA
if /i "%CURRENT_FOLDER%" neq "scripts" (
  rem seem to we are running from the root dir)
  set ROOT_DIR=.
)

set OUTPUT_DIR=%ROOT_DIR%\%BUILD_DIR%

rem # check if output folder exists, if not create it
if not exist "%OUTPUT_DIR%" (
    mkdir "%OUTPUT_DIR%"
)

set ARCH=x64
pushd %OUTPUT_DIR%
%ROOT_DIR%\bin\windows\bin\cmake.exe .. -A %ARCH% -DVAMIGA=ON

@echo on ""
echo  "------- CMake generation finished ------- "
choice /M "Open 'quaesar.sln' in Visual Studio? :> " /T 5 /D N
if errorlevel 2 goto :after_run
if errorlevel 1 (
  start quaesar.sln
)
:after_run
popd

endlocal
popd