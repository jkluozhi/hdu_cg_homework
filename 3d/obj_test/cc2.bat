@echo off
set MSVC=C:/Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207
cd /d "%~dp0\.."
"%MSVC%\bin\Hostx86\x86\dumpbin.exe" /symbols obj_test\chk_cam.obj | findstr /i "view"
