@echo off
setlocal
set MSVC=C:/Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207
set SDKI=C:/Program Files (x86)\Windows Kits\10\Include\10.0.26100.0
set SDKL=C:/Program Files (x86)\Windows Kits\10\Lib\10.0.26100.0
set INCLUDE=%MSVC%\include;%SDKI%\ucrt;%SDKI%\um;%SDKI%\shared;%SDKI%\winrt
set LIB=%MSVC%\lib\x86;%SDKL%\ucrt\x86;%SDKL%\um\x86
cd /d "%~dp0\.."
"%MSVC%\bin\Hostx86\x86\cl.exe" /nologo /c /EHsc /W3 /Foobj_test\ test_objload_cubetorus.cpp objload.cpp
"%MSVC%\bin\Hostx86\x86\link.exe" /nologo /subsystem:console /out:obj_test\test_ct.exe obj_test\test_objload_cubetorus.obj obj_test\objload.obj
obj_test\test_ct.exe
echo EXIT=%ERRORLEVEL%
