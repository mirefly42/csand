#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

void checked_printf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    if (vprintf(fmt, ap) < 0) {
        perror("printf");
        exit(1);
    }
    va_end(ap);
}

int main(void) {
    int c;
    while ((c = getchar()) != EOF) {
       checked_printf("%d,", c);
    }

    if (ferror(stdin)) {
        perror("getchar");
        return 1;
    }

    checked_printf("\n");

    return 0;
}
