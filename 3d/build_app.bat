@echo off
rem ===== main app build (Win32) =====
rem compiles 3d.cpp + math3d/camera/raster/clip3d -> obj_test\3d_app.exe
rem needs glut32.dll next to the exe at runtime
setlocal
set MSVC=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207
set SDKI=C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0
set SDKL=C:\Program Files (x86)\Windows Kits\10\Lib\10.0.26100.0
set INCLUDE=%MSVC%\include;%SDKI%\ucrt;%SDKI%\um;%SDKI%\shared;%SDKI%\winrt;C:\temp\glutinc
set LIB=%MSVC%\lib\x86;%SDKL%\ucrt\x86;%SDKL%\um\x86;C:\temp\glutlib
cd /d "%~dp0"

if not exist obj_test mkdir obj_test
"%MSVC%\bin\Hostx86\x86\cl.exe" /nologo /c /EHsc /W3 /Foobj_test\ 3d.cpp math3d.cpp camera.cpp raster.cpp clip3d.cpp objload.cpp curves.cpp
if errorlevel 1 goto done
"%MSVC%\bin\Hostx86\x86\link.exe" /nologo /subsystem:console /out:obj_test\3d_app.exe obj_test\3d.obj obj_test\math3d.obj obj_test\camera.obj obj_test\raster.obj obj_test\clip3d.obj obj_test\objload.obj obj_test\curves.obj glut32.lib opengl32.lib glu32.lib
:done
echo EXITCODE=%ERRORLEVEL%
