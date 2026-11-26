@echo off
set MSVC=C:/Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207
set SDKI=C:/Program Files (x86)\Windows Kits\10\Include\10.0.26100.0
set INCLUDE=%MSVC%\include;%SDKI%\ucrt;%SDKI%\um;%SDKI%\shared;%SDKI%\winrt;C:/temp/glutinc
cd /d "%~dp0\.."
"%MSVC%\bin\Hostx86\x86\cl.exe" /nologo /c /EHsc /W3 /Foobj_test\chk_cam2.obj camera.cpp 2>&1
echo ===DONE===
