
main.bin: index.c 
	gcc -O2 -Wall -pedantic index.c -o main.bin

clean:
	rm -f main.o main.bin
