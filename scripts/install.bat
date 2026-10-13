@echo off
setlocal

powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0install.ps1" %*
set "EXIT_CODE=%ERRORLEVEL%"

echo %CMDCMDLINE% | findstr /i /c:"%~nx0" >nul && pause

exit /b %EXIT_CODE%
