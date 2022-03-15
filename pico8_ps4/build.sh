# Package information
PKG_TITLE="PS4 P-8"
PKG_VERSION="1.00"
PKG_ASSETS="assets"
PKG_TITLE_ID="BREW34965"
PKG_CONTENT_ID="IV0000-BREW34965_00-OLIPS4PICO800000"

# Libraries to link in
libraries="-lc -lkernel -lc++ -lSceUserService -lSceVideoOut -lSceAudioOut -lScePad -lSceSysmodule -lSceFreeType -lSDL2 -lSDL2_image -lSceNet -lSceHttp -lSceSsl"

extra_flags='-I"../lua" -I"../libfixmath" -D__PS4__'

# Read the script arguments into local vars
intdir=ps4_int
targetname=pico8_ps4
outputPath=..\

outputElf=$intdir/$targetname.elf
outputOelf=$intdir/$targetname.oelf

if [ -d $intdir ]; then rm -r $intdir; fi
mkdir $intdir


ls *.c >/dev/null 2>&1
if [ $? -eq 0 ]; # if files with .c extension found then compile them
then
	# Compile object files for all the source files
	for f in *.c;
	do
		clang --target=x86_64-pc-freebsd12-elf -fPIC -funwind-tables -I"$OO_PS4_TOOLCHAIN/include" -I"$OO_PS4_TOOLCHAIN/include/c++/v1" $extra_flags -c -o $intdir/${f%.c}.o $f

		if [ ! $? -eq 0 ]; then exit 1; fi;

	done
fi


ls *.cpp >/dev/null 2>&1
if [ $? -eq 0 ]; # if files with .cpp extension found then compile them
then
	for f in *.cpp;
	do
		clang++ --target=x86_64-pc-freebsd12-elf -fPIC -funwind-tables -I"$OO_PS4_TOOLCHAIN/include" -I"$OO_PS4_TOOLCHAIN/include/c++/v1" -I"../lua" -I"../libfixmath" -D__PS4__ -c -o $intdir/${f%.cpp}.o $f

		if [ ! $? -eq 0 ]; then exit 1; fi;

	done
fi


# Link the input ELF
ld.lld -m elf_x86_64 -pie --script "$OO_PS4_TOOLCHAIN/link.x" --eh-frame-hdr -o "$outputElf" "-L$OO_PS4_TOOLCHAIN/lib" $libraries --verbose "$OO_PS4_TOOLCHAIN/lib/crt1.o" $intdir/*.o ../libfixmath/$intdir/*.o ../lua/$intdir/*.o

if [ ! $? -eq 0 ]; then exit 1; fi;

# Create the eboot
$OO_PS4_TOOLCHAIN/bin/linux/create-fself -in "$outputElf" --out "$outputOelf" --eboot "eboot.bin" --paid 0x3800000000000011

# Eboot cleanup
cp "eboot.bin" $outputPath/eboot.bin
rm "eboot.bin"


# Create param.sfo
cd ..
$OO_PS4_TOOLCHAIN/bin/linux/PkgTool.Core sfo_new sce_sys/param.sfo
$OO_PS4_TOOLCHAIN/bin/linux/PkgTool.Core sfo_setentry sce_sys/param.sfo APP_TYPE --type Integer --maxsize 4 --value 1
$OO_PS4_TOOLCHAIN/bin/linux/PkgTool.Core sfo_setentry sce_sys/param.sfo APP_VER --type Utf8 --maxsize 8 --value $PKG_VERSION
$OO_PS4_TOOLCHAIN/bin/linux/PkgTool.Core sfo_setentry sce_sys/param.sfo ATTRIBUTE --type Integer --maxsize 4 --value 0
$OO_PS4_TOOLCHAIN/bin/linux/PkgTool.Core sfo_setentry sce_sys/param.sfo CATEGORY --type Utf8 --maxsize 4 --value "gd"
$OO_PS4_TOOLCHAIN/bin/linux/PkgTool.Core sfo_setentry sce_sys/param.sfo CONTENT_ID --type Utf8 --maxsize 48 --value $PKG_CONTENT_ID
$OO_PS4_TOOLCHAIN/bin/linux/PkgTool.Core sfo_setentry sce_sys/param.sfo SYSTEM_VER --type Integer --maxsize 4 --value 0
$OO_PS4_TOOLCHAIN/bin/linux/PkgTool.Core sfo_setentry sce_sys/param.sfo TITLE --type Utf8 --maxsize 128 --value "$PKG_TITLE"
$OO_PS4_TOOLCHAIN/bin/linux/PkgTool.Core sfo_setentry sce_sys/param.sfo TITLE_ID --type Utf8 --maxsize 12 --value $PKG_TITLE_ID
$OO_PS4_TOOLCHAIN/bin/linux/PkgTool.Core sfo_setentry sce_sys/param.sfo VERSION --type Utf8 --maxsize 8 --value $PKG_VERSION

# Get a list of assets for packaging
module_files="`find sce_module -type f -printf "%p "`"

asset_audio_files="`find assets/audio/ -type f -printf "%p "`"

asset_fonts_files="`find assets/fonts/ -type f -printf "%p "`"

asset_images_files="`find assets/images/ -type f -printf "%p "`"

asset_misc_files="`find assets/misc/ -type f -printf "%p "`"

asset_videos_files="`find assets/videos/ -type f -printf "%p "`"


# Create gp4
$OO_PS4_TOOLCHAIN/bin/linux/create-gp4 -out pkg.gp4 --content-id=$PKG_CONTENT_ID --files "eboot.bin sce_sys/about/right.sprx sce_sys/param.sfo sce_sys/icon0.png $module_files $asset_audio_files $asset_fonts_files $asset_images_files $asset_misc_files $asset_videos_files"

# Create pkg
$OO_PS4_TOOLCHAIN/bin/linux/PkgTool.Core pkg_build pkg.gp4 .

if [ -f pico8.pkg ]; then rm -r pico8.pkg; fi
mv IV0000-BREW34965_00-OLIPS4PICO800000.pkg pico8.pkg
