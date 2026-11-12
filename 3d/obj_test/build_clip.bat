@echo off
setlocal
set MSVC=C:/Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207
set SDKI=C:/Program Files (x86)\Windows Kits\10\Include\10.0.26100.0
set SDKL=C:/Program Files (x86)\Windows Kits\10\Lib\10.0.26100.0
set INCLUDE=%MSVC%\include;%SDKI%\ucrt;%SDKI%\um;%SDKI%\shared;%SDKI%\winrt;C:/temp/glutinc
set LIB=%MSVC%\lib\x86;%SDKL%\ucrt\x86;%SDKL%\um\x86;C:/temp/glutlib
cd /d "%~dp0\.."
"%MSVC%\bin\Hostx86\x86\cl.exe" /nologo /c /EHsc /W3 /Foobj_test\ test_clip.cpp
"%MSVC%\bin\Hostx86\x86\link.exe" /nologo /subsystem:console /out:obj_test\test_clip.exe obj_test\test_clip.obj obj_test\math3d.obj obj_test\camera.obj obj_test\raster.obj obj_test\clip3d.obj glut32.lib opengl32.lib glu32.lib
obj_test\test_clip.exe
