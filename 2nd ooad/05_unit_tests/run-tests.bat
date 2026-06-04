@echo off
setlocal

cd /d "%~dp0"

..\..\cpp\run-tests.bat %*
exit /b %errorlevel%
