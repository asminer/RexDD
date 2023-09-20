#ifndef PARSER_H
#define PARSER_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>


typedef struct
{
    FILE* inf;          // input file
    bool pclose_inf;    // inf needs pclose if true, or fclose if false

} file_reader;

void init_file_reader(file_reader* fr, const char* filename, char comp);

void free_file_reader(file_reader* fr);

static inline bool eof(file_reader* fr) {
    return feof(fr->inf);
}

static inline int get(file_reader* fr) {
    return fgetc(fr->inf);
}

static inline unsigned read_unsigned(file_reader* fr) {
    unsigned x;
    fscanf(fr->inf, "%u", &x);
    return x;
}

static inline void unget(file_reader* fr, int c) {
    ungetc(c, fr->inf);
}

static inline unsigned read(void* ptr, unsigned bytes, file_reader* fr) {
    return (unsigned)fread(ptr, 1, bytes, fr->inf);
}


typedef struct
{
    file_reader* FR;        // input file reader
    unsigned inbits;        // number of inbits of minterms
    unsigned outbits;       // number of outbits of minterms
    unsigned long numf;     // number of minterms

    // only used for .bin file parser
    unsigned long bits;
    unsigned bits_avail;
    
} parser;

void init_parser(parser* p, file_reader* fr);

void free_parser(parser* p);

void read_header_pla(parser* p);

bool read_minterm_pla(parser* p, char* input_bits, char* out_terminal);

void read_header_bin(parser* p);

bool read_minterm_bin(parser* p, char* input_bits, char* out_terminal);


/*=======================================================================================================
 *                                          Some helper functions
 *=======================================================================================================*/
static inline bool peof(parser* p) {
    return eof(p->FR);
}

static inline int pget(parser* p) {
    return get(p->FR);
}

static inline void punget(parser* p, int c) {
    unget(p->FR, c);
}

static inline unsigned pread_unsigned(parser* p) {
    return read_unsigned(p->FR);
}

static inline unsigned pread(void* ptr, unsigned bytes, parser* p) {
    return read(ptr, bytes, p->FR);
}

static inline int skip_until(parser* p, char x) {
    for(;;) {
        int c = pget(p);
        if (c == x)     return x;
        if (c == EOF)   return EOF;
    }
}

static inline bool read_le(parser* p, unsigned long* u) {
    return (8 == pread(u, 8, p));
}

static inline void check_bitstream(parser* p) {
    if (p->bits_avail == 0) {
        if (!read_le(p, &(p->bits))) printf("Unexpected EOF\n");
        p->bits_avail = sizeof(unsigned long) * 8;
    }
}

static inline bool read_bit2(parser* p) {
    check_bitstream(p);
    unsigned long bp = p->bits & 0x03;
    p->bits >>= 2;
    p->bits_avail -= 2;
    switch (bp)
    {
    case 1: return false;
    case 2: return true;
    default: printf("unexpected bit value\n");
    }
    return 0;
}

static inline bool read_bit1(parser* p) {
    check_bitstream(p);
    unsigned long bp = p->bits & 0x01;
    p->bits >>= 1;
    p->bits_avail -= 1;
    return bp;
}

// determine file type
static inline bool matches(const char* ext, char format, char comp) {
    const char* ext1;
    const char* ext2;
    unsigned ext1len;
    switch (format) {
        case 'b':   ext1 = ".bin";  ext1len = 4;  break;
        default:    ext1 = ".pla";  ext1len = 4;  break;
    }
    switch (comp) {
        case 'x':   ext2 = ".xz";   break;
        case 'g':   ext2 = ".gz";   break;
        case 'b':   ext2 = ".bz2";  break;
        default:    ext2 = "";      break;
    }

    if (strncmp(ext1, ext, ext1len)) return false;
    return 0 == strcmp(ext2, ext+ext1len);
}

// get the file's type information into format and comp
static inline void file_type(const char* pathname, char* type)
{
    if (0==pathname) return;
    const char* formats = "pb";
    const char* comps   = " xgb";
    unsigned i, f, c;
    for (i=0; pathname[i]; ++i) {
        if ('.' != pathname[i]) continue;

        for (f=0; formats[f]; f++) {
            type[0] = formats[f];
            for (c=0; comps[c]; c++) {
                type[1] = comps[c];
                if (matches(pathname+i, type[0], type[1])) return;
            }
        }
    }
    printf("Could not determine the file type for input file %s\n", pathname);
    return;
}

#endif