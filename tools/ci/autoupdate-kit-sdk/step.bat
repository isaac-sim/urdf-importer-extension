@echo off

call "%~dp0..\..\..\repo.bat" kit_autoupdate update %*
if %errorlevel% neq 0 ( exit /b %errorlevel% )
