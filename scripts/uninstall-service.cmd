@echo off
set SERVICE_NAME=UndyingTerminal

sc stop "%SERVICE_NAME%"
sc delete "%SERVICE_NAME%"
