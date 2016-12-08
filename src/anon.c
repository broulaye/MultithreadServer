#include "anon.h"

typedef struct {
	void *addr;
	struct list_elem el;
} block;

void allocanon(struct list *l) {
	block b;
	b.addr = mmap(NULL, BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_ANONYMOUS, -1, 0);
	list_push_front(l, &b.el);
}

void freeanon(struct list *l) {
	struct list_elem *elem = list_pop_front(l);
	block *bp = list_entry(elem, block, el);
	munmap(bp->addr, BLOCK_SIZE);
}
