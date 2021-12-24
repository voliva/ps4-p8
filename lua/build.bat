SETLOCAL EnableDelayedExpansion

Rem Package information
set PKG_TITLE="OpenOrbis Hello World Sample"
set PKG_VERSION="1.00"
set PKG_ASSETS="assets"
set PKG_TITLE_ID="BREW00083"
set PKG_CONTENT_ID="IV0000-BREW00083_00-HELLOWORLDLUA000"

Rem Libraries to link in
set libraries=-lc -lkernel -lc++

Rem set extra_flags=

Rem Read the script arguments into local vars
set intdir=%1
set targetname=%~2
set outputPath=%3

set outputElf=%intdir%\%targetname%.elf
set outputOelf=%intdir%\%targetname%.oelf

@mkdir %intdir%

Rem Compile object files for all the source files
for %%f in (*.c) do (
    clang --target=x86_64-pc-freebsd12-elf -fPIC -funwind-tables -I"%OO_PS4_TOOLCHAIN%\\include" -I"%OO_PS4_TOOLCHAIN%\\include\\c++\\v1" %extra_flags% -c -o %intdir%\%%~nf.o %%~nf.c
)

for %%f in (*.cpp) do (
    clang++ --target=x86_64-pc-freebsd12-elf -fPIC -funwind-tables -I"%OO_PS4_TOOLCHAIN%\\include" -I"%OO_PS4_TOOLCHAIN%\\include\\c++\\v1" %extra_flags% -c -o %intdir%\%%~nf.o %%~nf.cpp
)
