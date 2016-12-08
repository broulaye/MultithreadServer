#include "runloop.h"
#include <stdio.h>

void runloop() {
	int terminate = 1;
	time_t start, end;
	start = time(&start);
	while (terminate) {
		end = time(&end);
		if (difftime(end, start) >= 15.0)
			terminate = 0;
	}
}

int main (int ac, char** av) {
	runloop();
	printf("done looping\n");
}
