mkdir -p bin/
gcc -o bin/z64yartool -Os -s -flto -DNDEBUG -Wall -Wextra -Wno-unused-function -std=c99 -pedantic -Iinclude -Iexoquant src/*.c  exoquant/*.c -lm

