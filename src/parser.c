#include "parser.h"

/*=======================================================================================================
 *                                  parser related funcitons
 *=======================================================================================================*/

void init_file_reader(file_reader* fr, const char* filename, char comp)
{
    if ((comp != 'x') && (comp != 'g') && (comp != 'b')) {
        // unknown compression, so don't
        comp = ' ';
    }
    if(comp == ' ') {
        fr->pclose_inf = false;
        if (filename) {
            fr->inf = fopen(filename, "r");
            if (!fr->inf) {
                printf("Could not fopen file %s\n", filename);
                exit(1);
            }
        } else {
            fr->inf = stdin;
        }
        return;
    }
    // now we will use popen
    fr->pclose_inf = true;
    
    char* zcat = 0;
    switch (comp)
    {
    case 'x': zcat = "xzcat";   break;
    case 'g': zcat = "gzcat";   break;
    case 'b': zcat = "bzcat";   break;
    default:    zcat = "cat";
    }

    if (!filename) {
        fr->inf = popen(zcat, "r");
        return;
    }

    char buffer[256];
    snprintf(buffer, 256, "%s %s", zcat, filename);
    fr->inf = popen(buffer, "r");
}

void free_file_reader(file_reader* fr)
{
    if (fr->inf) {
        if (fr->pclose_inf) {
            pclose(fr->inf);
        } else {
            fclose(fr->inf);
        }
    }
}

void init_parser(parser* p, file_reader* fr)
{
    p->FR = fr;
    if (!p->FR) {
        printf("file reader is null\n");
        exit(1);
    }
    p->inbits = 0;
    p->outbits = 0;

    p->numf = 0;
    p->bits = 0;
    p->bits_avail = 0;
}

void free_parser(parser* p)
{
    free_file_reader(p->FR);
    p->inbits = 0;
    p->outbits = 0;

    p->numf = 0;
    p->bits = 0;
    p->bits_avail = 0;
}

void read_header_pla(parser* p)
{
    int next = 0;
    for(;;) {
        // read lines while first character is'.'
        next = pget(p);
        if (next == EOF) {
            printf("Unexpected EOF.\n");
            break;
        }
        if (next == '#') {
            skip_until(p, '\n');
            continue;
        }
        if (next != '.') {
            punget(p, next);
            break;
        }

        // now we get '.'

        next = pget(p);
        if (next == 'i') {
            p->inbits = pread_unsigned(p);
        }
        if (next == 'o') {
            p->outbits = pread_unsigned(p);
        }
        if (next == 'p') {
            p->numf = pread_unsigned(p);    // number of minterms
        }
        skip_until(p,'\n');
    }
}

bool read_minterm_pla(parser* p, char* input_bits, char* out_terminal)
{
    int c;
    for (;;) {
        c = pget(p);
        if (c == EOF) return false;
        if (c == '.') {
            skip_until(p, '\n');
        }
        break;
    }

    // non . line it's a minterm
    input_bits[0] = 0;
    input_bits[1] = (char)c;
    unsigned u;
    for (u=2; u<=p->inbits; u++) {
        c = pget(p);
        if (c == EOF) return false;
        input_bits[u] = (char)c;
    }
    input_bits[u] = 0;

    // read output bits and build terminal value
    bool all_dash = true;
    unsigned t = 0;
    for (;;) {
        c = pget(p);
        if (c == EOF) return false;
        if (c == '\n') break;
        if ((c == ' ') || (c == '\t') || (c == '\r')) continue;
        t *= 2;
        t += ((c == '1')? 1:0);
        if (!((c == '-') || (c == '2'))) all_dash = false;
    }
    if (all_dash) {
        out_terminal[0] = '0';
    } else {
        out_terminal[0] = (char)('0' + t);
        if (out_terminal[0] == '6') {
            out_terminal[0] = '5';
        }
    }
    return true;
}

void read_header_bin(parser* p)
{
    unsigned ob = 3;
    unsigned long numbits = 0 , numr = 0, numdc = 0;
    read_le(p, &numbits);
    read_le(p, &(p->numf));
    read_le(p, &numr);
    read_le(p, &numdc);

    p->inbits = (unsigned)((numbits-ob)/2);
    p->outbits = ob;
}

bool read_minterm_bin(parser* p, char* input_bits, char* out_terminal)
{
    if (p->numf == 0) return false;
    // clear bitsstream
    p->bits_avail = 0;

    input_bits[0] = 0;
    unsigned i;
    for (i=1; i<=p->inbits; i++) {
        input_bits[i] = '0' + (read_bit2(p) ? 1 : 0);
    }
    input_bits[i] = 0;

    const unsigned char w1 = read_bit1(p) ? 1 : 0;
    unsigned char w2 = read_bit1(p) ? 1 : 0;
    const unsigned char w3 = read_bit1(p) ? 1 : 0;
    if (w1 && w3) {
        out_terminal[0] = '0';
    } else {
        out_terminal[0] = (char)('0' + w1*4 + w2*2 + w3);
        if (out_terminal[0] == '6') {
            out_terminal[0] = '5';
        }
    }
    p->numf--;
    return true;
}

void reverse(char* input, unsigned nbits){
    unsigned L = 0;
    unsigned H = nbits-1;

    while (L < H) {
        char t = input[L];
        input[L] = input[H];
        input[H] = t;
        L++;
        H--;
    }
}