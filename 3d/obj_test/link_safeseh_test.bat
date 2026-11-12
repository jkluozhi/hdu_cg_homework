@echo off
setlocal
set MSVC=C:/Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207
set SDKL=C:/Program Files (x86)\Windows Kits\10\Lib\10.0.26100.0
set LIB=%MSVC%\lib\x86;%SDKL%\ucrt\x86;%SDKL%\um\x86;C:/temp/glutlib
cd /d "%~dp0\.."
echo === 1. /SAFESEH (IDE default) ===
"%MSVC%\bin\Hostx86\x86\link.exe" /nologo /SAFESEH /subsystem:console /out:obj_test\se_test1.exe obj_test\3d.obj obj_test\math3d.obj obj_test\camera.obj obj_test\raster.obj obj_test\clip3d.obj obj_test\objload.obj glut32.lib opengl32.lib glu32.lib
echo EXIT=%ERRORLEVEL%
echo === 2. /SAFESEH:NO (the fix) ===
"%MSVC%\bin\Hostx86\x86\link.exe" /nologo /SAFESEH:NO /subsystem:console /out:obj_test\se_test2.exe obj_test\3d.obj obj_test\math3d.obj obj_test\camera.obj obj_test\raster.obj obj_test\clip3d.obj obj_test\objload.obj glut32.lib opengl32.lib glu32.lib
echo EXIT=%ERRORLEVEL%
