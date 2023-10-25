@echo off

call "%~dp0..\..\..\repo.bat" kit_autoupdate release %*
if %errorlevel% neq 0 ( exit /b %errorlevel% )
