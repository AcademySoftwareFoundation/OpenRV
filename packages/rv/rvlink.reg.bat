@echo off
REM Check for administrative privileges
net session >nul 2>&1
if %errorLevel% == 0 (
    echo Success: Administrative privileges confirmed.
) else (
    echo Failure: Current permissions to execute this .BAT file are inadequate.
    echo Please run this script as an administrator.
    pause
    exit /b    exit /b
)

cd /d %~dp0
pushd ..

REM Define the path to the executable
set "scriptDir=%cd%"
set "rvExePath=%scriptDir%\bin\rv.exe"

REM Check if the executable exists
if not exist "%rvExePath%" (
    echo Executable file %rvExePath% does not exist. Exiting script.
    popd
    pause
    exit /b)

@echo on
REM Create registry keys and values
reg add "HKCR\rvlink" /f
reg add "HKCR\rvlink" /ve /d "URL:RV Protocol" /f
reg add "HKCR\rvlink" /v "URL Protocol" /d "" /f

reg add "HKCR\rvlink\DefaultIcon" /f
reg add "HKCR\rvlink\DefaultIcon" /ve /d "rv.exe,1" /f

reg add "HKCR\rvlink\shell" /f
reg add "HKCR\rvlink\shell\open" /f

reg add "HKCR\rvlink\shell\open\command" /f
reg add "HKCR\rvlink\shell\open\command" /ve /d "\"%rvExePath%\" \"%%1\"" /f

popd
pause

