@echo off
echo [Step 1] Starting server...
start /B build\undying-terminal-server.exe

timeout 3

echo [Step 2] Registering terminal...
build\undying-terminal-terminal.exe --register test-client

if errorlevel 1 (
    echo FAILED: Terminal registration failed
    goto :end
)
echo OK: Terminal registered

timeout 2

echo [Step 3] Connecting client...
build\undying-terminal.exe --connect 127.0.0.1 2022 test-client

if errorlevel 1 (
    echo FAILED: Client connection failed
    goto :end
)
echo OK: Client connected

:end
echo.
echo Press any key to clean up...
pause > nul

taskkill /F /IM undying-terminal-server.exe /T undying-terminal-terminal.exe 2>nul
echo Done.
