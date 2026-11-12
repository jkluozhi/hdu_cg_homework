@echo off
rem ===== T1-T8 命令行验证（Win32）=====
rem 只编译 math3d / camera / raster + 测试主体，不含 3d.cpp（避免 main 冲突）
rem 依赖 C:\temp\glutinc 与 C:\temp\glutlib（GLUT 头文件与库的 ASCII 副本）
setlocal
set MSVC=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207
set SDKI=C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0
set SDKL=C:\Program Files (x86)\Windows Kits\10\Lib\10.0.26100.0
set INCLUDE=%MSVC%\include;%SDKI%\ucrt;%SDKI%\um;%SDKI%\shared;%SDKI%\winrt;C:\temp\glutinc
set LIB=%MSVC%\lib\x86;%SDKL%\ucrt\x86;%SDKL%\um\x86;C:\temp\glutlib
cd /d "%~dp0"

if not exist obj_test mkdir obj_test
"%MSVC%\bin\Hostx86\x86\cl.exe" /nologo /c /EHsc /W3 /Foobj_test\ math3d.cpp camera.cpp raster.cpp clip3d.cpp test_t1_t8.cpp
if errorlevel 1 goto done
"%MSVC%\bin\Hostx86\x86\link.exe" /nologo /subsystem:console /out:obj_test\test_t1_t8.exe obj_test\test_t1_t8.obj obj_test\math3d.obj obj_test\camera.obj obj_test\raster.obj obj_test\clip3d.obj glut32.lib opengl32.lib glu32.lib
if errorlevel 1 goto done
obj_test\test_t1_t8.exe
:done
echo EXITCODE=%ERRORLEVEL%
