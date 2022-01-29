SETLOCAL EnableDelayedExpansion

Rem Read the script arguments into local vars
set intdir=ps4_int

@mkdir %intdir%

Rem Compile object files for all the source files
for %%f in (*.c) do (
    clang --target=x86_64-pc-freebsd12-elf -fPIC -funwind-tables -I"%OO_PS4_TOOLCHAIN%\\include" -I"%OO_PS4_TOOLCHAIN%\\include\\c++\\v1" %extra_flags% -c -o %intdir%\%%~nf.o %%~nf.c
)

for %%f in (*.cpp) do (
    clang++ --target=x86_64-pc-freebsd12-elf -fPIC -funwind-tables -I"%OO_PS4_TOOLCHAIN%\\include" -I"%OO_PS4_TOOLCHAIN%\\include\\c++\\v1" %extra_flags% -c -o %intdir%\%%~nf.o %%~nf.cpp
)
