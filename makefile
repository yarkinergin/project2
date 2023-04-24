all: mps

mps: mps.c
	gcc -Wall -g -o mps mps.c -lpthread -lm

clean:
	rm -fr mps *~ output*