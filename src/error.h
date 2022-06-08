#ifndef ERROR_H
#define ERROR_H

/*
 *  Fatal runtime error handling.
 *
 *  For now - write an error message to stderr, and exit.
 *  In the future - allow users to provide an alternative
 *  C function (pointer) to catch errors, with signature
 *
 *      error_func(const char* f, unsigned l, const char* format, va_list p);
 *
 *  This will probably require a "libary initialization" function.
 *
 */

void rexdd_error(const char* file, unsigned line, const char* format, ...);


#endif
