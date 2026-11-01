@echo off
call "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Auxiliary/Build/vcvarsall.bat" x64 >nul 2>&1
cl /nologo /O2 /EHsc stackdump.cpp /Fe:stackdump.exe dbghelp.lib
