#include "loadavg.h"

void getLoadAvg() {
	FILE *loadavg = fopen("/proc/loadavg", "r");
	FILE *output = fopen("loadavg.json", "w");
	if (output == NULL || loadavg == NULL) {
		fprintf(stderr, "Unable to open file\n");
		return;
	}

	char buf[1024];
	fgets(buf, 1024, loadavg);
	fclose(loadavg);
	float loadavg_num1, loadavg_num2, loadavg_num3;
	int running_thread_num, total_thread_num;
	sscanf(buf, "%f %f %f %d %*c %d", &loadavg_num1, &loadavg_num2, &loadavg_num3, &running_thread_num, &total_thread_num);
	fprintf(output, "{\"total_threads\": \"%d\", \"loadavg\": [\"%0.2f\", \"%0.2f\", \"%0.2f\"], \"running_threads\": \"%d\"", total_thread_num, loadavg_num1, loadavg_num2, loadavg_num3, running_thread_num);
}
