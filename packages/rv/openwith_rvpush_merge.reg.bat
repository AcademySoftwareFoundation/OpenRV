@echo off
REM Check for administrative privileges
net session >nul 2>&1
if %errorLevel% == 0 (
    echo Success: Administrative privileges confirmed.
) else (
    echo Failure: Current permissions to execute this .BAT file are inadequate.
    echo Please run this script as an administrator.
    pause
    exit /b
)

cd /d %~dp0
pushd ..

REM Define the path to the executable
set "scriptDir=%cd%"
set "rvpushExePath=%scriptDir%\bin\rvpush.exe"

REM Check if the executable exists
if not exist "%rvpushExePath%" (
    echo Executable file %rvpushExePath% does not exist. Exiting script.
    popd
    pause
    exit /b
)

@echo on
REM Create registry keys and values
reg add "HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\rvpush.exe" /f
reg add "HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\rvpush.exe" /ve /d "%rvpushExePath%" /f
reg add "HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\rvpush.exe" /v "Path" /d "%scriptDir%\bin" /f

reg add "HKCR\Applications\rvpush.exe\shell\open" /f
reg add "HKCR\Applications\rvpush.exe\shell\open" /v "FriendlyAppName" /d "RVPUSH" /f

reg add "HKCR\Applications\rvpush.exe\shell\open\command" /f
reg add "HKCR\Applications\rvpush.exe\shell\open\command" /ve /d "\"%rvpushExePath%\" -tag \"openwith\" \"merge\" \"%%1\"" /f

reg add "HKCR\Applications\rvpush.exe\SupportedTypes" /f
for %%A in (3gp aces aif aifc aiff ari au avi bmp bw cin cineon cur cut dds dpx exr gif hdr ico iff j2c j2k jp2 jpeg jpg jpt lbm lif lmp mdl mov movieproc mp4 mraysubfile openexr pbm pcd pcx pdf pgm pic png ppm psd qt rgb rgba rgbe rla rpf sgi shd sm snd stdinfb sxr targa tdl tex tga tif tiff tpic tx txr txt wal wav yuv z zfile) do (
    reg add "HKCR\Applications\rvpush.exe\SupportedTypes" /v %%A /d "" /f
)

popd
pause

