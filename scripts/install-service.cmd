@echo off
set SERVICE_NAME=UndyingTerminal
set DISPLAY_NAME=Undying Terminal
set BIN_PATH="%~dp0..\build\undying-terminal-server.exe"

sc create "%SERVICE_NAME%" binPath= %BIN_PATH% start= auto DisplayName= "%DISPLAY_NAME%"
sc description "%SERVICE_NAME%" "Re-connectable secure remote shell"
sc start "%SERVICE_NAME%"
