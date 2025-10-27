@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
call subst v: D:\GitRepos\Direct3D
set path=V:\misc;%path%
V:
cd code
