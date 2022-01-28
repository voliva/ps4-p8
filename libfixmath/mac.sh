intdir=mac_build

mkdir $intdir

for file in *.c; do
  filename="${file%.*}"
  echo "clang -c -o \"$intdir/$filename.o\" $file"
  clang -c -o "$intdir/$filename.o" $file
done
