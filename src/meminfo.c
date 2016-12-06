#include "meminfo.h"

void getMemInfo() {
	FILE *meminfo = fopen("/proc/meminfo", "r");
	FILE *output = fopen("meminfo.json", "w");

	if (output == NULL || meminfo == NULL) {
		fprintf(stderr, "Unable to open file\n");
		return;
	}

	fprintf(output, "{");
	char buf[1024];
	while(fgets(buf, 1024, meminfo) != NULL) {
		char name[32];
		int val;
		sscanf(buf, "%s: %d", name, &val);
		int col_ind = strlen(name);
		name[col_ind - 1] = '\0';
		fprintf(output, "\"%s\": \"%d\", ", name, val);
	}
	fprintf(output, "}");

	fclose(meminfo);
}
