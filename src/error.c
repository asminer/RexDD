
#include "error.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

void rexdd_default_error(const char* file, unsigned line,
        const char* format, va_list ap)
{
    fprintf(stderr, "\nFatal error in rexdd library!\n");
    fprintf(stderr, "    source file: %s\n", file);
    fprintf(stderr, "    line number: %u\n", line);
    fprintf(stderr, "    description: ");
    vfprintf(stderr, format, ap);
    fputs("\n\n", stderr);
    fflush(stderr);
    exit(1);
}

void rexdd_error(const char* file, unsigned line, const char* format, ...)
{
    va_list ap;
    va_start(ap, format);

    // TBD - check for user-defined function and call it instead
    //

    rexdd_default_error(file, line, format, ap);
    va_end(ap);
}
