intdir=mac_build

mkdir $intdir

for file in *.c; do
  filename="${file%.*}"
  if [ "$file" -nt "$intdir/$filename.o" ]
  then
    echo "clang -I\"../libfixmath\" -c -o \"$intdir/$filename.o\" $file"
    clang -I"../libfixmath" -c -o "$intdir/$filename.o" $file
  fi
done
