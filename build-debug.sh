mkdir -p bin/
gcc -o bin/z64yartool -Og -g -Wall -Wextra -Wno-unused-function -lm -std=c99 -pedantic -Iinclude src/*.c

