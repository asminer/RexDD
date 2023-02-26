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

bool eof(file_reader* fr) {
    return feof(fr->inf);
}

int get(file_reader* fr) {
    return fgetc(fr->inf);
}

unsigned read_unsigned(file_reader* fr) {
    unsigned x;
    fscanf(fr->inf, "%u", &x);
    return x;
}

void unget(file_reader* fr, int c) {
    ungetc(c, fr->inf);
}

unsigned read(void* ptr, unsigned bytes, file_reader* fr) {
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


/*
 *  Some helper functions for parser
 */
bool peof(parser* p) {
    return eof(p->FR);
}

int pget(parser* p) {
    return get(p->FR);
}

void punget(parser* p, int c) {
    unget(p->FR, c);
}

unsigned pread_unsigned(parser* p) {
    return read_unsigned(p->FR);
}

unsigned pread(void* ptr, unsigned bytes, parser* p) {
    return read(ptr, bytes, p->FR);
}

int skip_until(parser* p, char x) {
    for(;;) {
        int c = pget(p);
        if (c == x)     return x;
        if (c == EOF)   return EOF;
    }
}

