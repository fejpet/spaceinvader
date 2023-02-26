all:
	gcc -I src/include -L src/lib  -o main main.cpp -lstdc++ -lmingw32 -lSDL2main -lSDL2