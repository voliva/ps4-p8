SETLOCAL EnableDelayedExpansion

set extra_flags=-I"..\\..\\lua" -I "..\\..\\lib\\include" -I"E:\development\vcpkg\installed\x64-windows\include"

Rem Read the script arguments into local vars
set intdir=build
set targetname=run_test.exe

IF EXIST %intdir% RMDIR /S /Q %intdir%
@mkdir %intdir%

Rem Compile object files for all the source files
clang++ -c -o %intdir%\cartridge.o ..\cartridge.cpp %extra_flags%
clang++ -c -o %intdir%\log.o ..\log.cpp %extra_flags%
clang++ -c -o %intdir%\http.o ..\http.cpp %extra_flags%
clang++ -c -o %intdir%\cartridge_test.o cartridge_test.cpp %extra_flags%

set lua_files=
for %%f in (..\..\lua\*.c) do set lua_files=!lua_files! .\%%f

Rem Get a list of object files for linking
set obj_files=

Rem Lua obj files
for %%f in (%intdir%\*.o) do set obj_files=!obj_files! .\%%f

clang %lua_files% %obj_files% -o "%targetname%" -llibcurl -L"E:\development\vcpkg\installed\x64-windows\lib"
