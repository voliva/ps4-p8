intdir=mac_build

mkdir $intdir

set -e # Exit script when any error happens

for file in *.cpp; do
  filename="${file%.*}"
  if [ "$file" -nt "$intdir/$filename.o" ]
  then
    echo "clang++ -c -o \"$intdir/$filename.o\" $file"
    clang++ -std=c++11 -I"../lua" -I"../libfixmath" -I"../lib/include" -c -o "$intdir/$filename.o" $file
  fi
done

if [ "$1" != "skiprun" ]; then
  echo "clang++ mac_build/*.o ../lua/mac_build/*.o ../libfixmath/mac_build/*.o -lSDL2 -lcurl -o \"$intdir/ps4-p8\""
  clang++ mac_build/*.o ../lua/mac_build/*.o ../libfixmath/mac_build/*.o -lSDL2 -lcurl -o "$intdir/ps4-p8"
  "./$intdir/ps4-p8"
fi
