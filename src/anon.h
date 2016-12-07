#ifndef ANON_H
#define ANON_H

#include <stdlib.h>
#include <sys/mman.h>
#include "list.h"

#define BLOCK_SIZE 256

void allocanon(struct list *l);
void freeanon(struct list *l);

#endif
