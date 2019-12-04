#define _GNU_SOURCE
#include "dlmall.h"

int main(){
    void *a = dalloc(32);
    void *b = dalloc(32);

    void *c = dalloc(32);

    void *d = dalloc(32);

    void *e = dalloc(32);

    void *f = dalloc(32);

    dfree(a);
    dfree(b);
    dfree(c);
    dfree(d);

    return 0;
}