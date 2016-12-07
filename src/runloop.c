#include "runloop.h"

void runloop() {
	int iter = 0;
	int terminate = 1;
	while (terminate) {
		if (iter >= 15) {
			terminate = 0;
		}
		else {
			iter++;
			sleep(1);
		}
	}
}
