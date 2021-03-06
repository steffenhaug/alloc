#include <stdio.h>
#include "alloc.h"

int main() {
    char *p1 = alloc(140);
    char *p2 = alloc(8);
    free(p1);

    hexdump();

    return 0;
}
