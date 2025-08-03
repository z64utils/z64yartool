mkdir -p bin/
i686-w64-mingw32.static-gcc -o bin/z64yartool.exe -Os -s -flto -DNDEBUG -Wall -Wextra -Wno-unused-function -std=c99 -pedantic -Iinclude src/*.c -lm

