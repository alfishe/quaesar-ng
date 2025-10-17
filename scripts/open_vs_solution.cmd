@echo off
pushd %~dp0
setlocal

set ROOT_DIR=%cd%\..\
set CMAKE_EXE=%ROOT_DIR%\bin\windows\bin\cmake.exe
set BUILD_DIR=build
set OUTPUT_DIR=%ROOT_DIR%\%BUILD_DIR%

if not exist "%OUTPUT_DIR%" (
    mkdir "%OUTPUT_DIR%"
)

rem set ARCH=x64
 set ARCH=Win32

pushd %OUTPUT_DIR%
%CMAKE_EXE% .. -A %ARCH% -DVAMIGA=ON

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