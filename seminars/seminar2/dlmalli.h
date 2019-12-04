#include <stdio.h>

struct head;

void *dalloc(size_t request);
void dfree(void *memory);
int printFlist();