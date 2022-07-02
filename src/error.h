#ifndef ERROR_H
#define ERROR_H

/*************************************************************************
 *
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


/*************************************************************************
 *
 *  Error check shorthand macros.
 *  These are always on.
 *
 */

#define rexdd_check1(T, F)      { if (!(T)) rexdd_error(__FILE__, __LINE__, F); }
#define rexdd_check2(T, F, P)   { if (!(T)) rexdd_error(__FILE__, __LINE__, F, P); }

/*************************************************************************
 *
 *  Sanity check macros.
 *  Turned on for debugging only
 *
 */

#ifdef NDEBUG
#define rexdd_sanity1(T, F)
#define rexdd_sanity2(T, F, P)
#else
#define rexdd_sanity1(T, F)         rexdd_check1(T, F)
#define rexdd_sanity2(T, F, P)      rexdd_check2(T, F, P)
#endif

#endif
