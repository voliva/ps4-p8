# Read the script arguments into local vars
intdir=ps4_int

mkdir $intdir


ls *.c >/dev/null 2>&1
if [ $? -eq 0 ]; # if files with .c extension found then compile them
then
	# Compile object files for all the source files
	for f in *.c;
	do
		clang --target=x86_64-pc-freebsd12-elf -fPIC -funwind-tables -I"../libfixmath" -I"$OO_PS4_TOOLCHAIN/include" -I"$OO_PS4_TOOLCHAIN/include/c++/v1" $extra_flags -c -o $intdir/${f%.c}.o $f
	done
fi


ls *.cpp >/dev/null 2>&1
if [ $? -eq 0 ]; # if files with .cpp extension found then compile them
then
	for f in *.cpp;
	do
		clang++ --target=x86_64-pc-freebsd12-elf -fPIC -funwind-tables -I"../libfixmath" -I"$OO_PS4_TOOLCHAIN/include" -I"$OO_PS4_TOOLCHAIN/include/c++/v1" $extra_flags -c -o $intdir/${f%.cpp}.o $f
	done
fi
