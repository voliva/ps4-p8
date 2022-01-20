cd ..
./mac.sh skiprun
cd test

set -e # Exit script when any error happens

includedFiles="../mac_build/cartridge.o ../mac_build/http.o ../mac_build/log.o"
clang++ cartridge_test.cpp $includedFiles ../../lua/mac_build/*.o -std=c++11 -lSDL2 -lcurl -I"../../lua" -o "cartridge_test.exe"
"./cartridge_test.exe"
