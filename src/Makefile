# for benchmarking
CFLAGS=-Wall -O3 -Werror -Wmissing-prototypes -fopenmp -std=gnu11
# for debugging
#CFLAGS=-Wall -O0 -g -Werror -Wmissing-prototypes -std=gnu11

LDLIBS=-lpthread -lrt

OBJ=list.o csapp.o loadavg.o meminfo.o

ALL=sysstatd

all: $(ALL)

sysstatd: sysstatd.o $(OBJ)
	gcc $(CFLAGS) -o sysstatd sysstatd.o $(OBJ) $(LDLIBS)
	
clean:
	rm -f *.o $(ALL)
