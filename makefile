all: mps mps_cv

mps: mps.c
	gcc -Wall -g -o mps mps.c -lpthread -lm

mps_cv: mps_cv.c
	gcc -Wall -g -o mps_cv mps_cv.c -lpthread -lm

clean:
	rm -fr mps_cv *~ output*