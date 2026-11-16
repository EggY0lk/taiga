@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
"C:\Qt\6.10.3\msvc2022_64\bin\windeployqt.exe" "%~dp0bin\Taiga.exe" --release --no-translations --compiler-runtime
