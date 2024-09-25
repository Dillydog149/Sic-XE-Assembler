#Dylan Bernier and Mihai Muntean
#Cssc4062 and Cssc4061
#570 Spring 24
#Assigment #2, XE Two Pass Assembler
#Makefile


CC = g++
CFLAGS = -std=c++11

main: main.o
	$(CC) $(CFLAGS) -o main main.o

main.o: main.cpp
	$(CC) $(CFLAGS) -c main.cpp

clean:
	rm -rf *.o main
