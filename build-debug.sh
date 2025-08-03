mkdir -p bin/
gcc -o bin/z64yartool -Og -g -Wall -Wextra -Wno-unused-function -std=c99 -pedantic -Iinclude src/*.c -lm

