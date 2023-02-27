#include "forest.h"
#include "parser.h"
#include "helpers.h"

#include <assert.h>

/*=======================================================================================================
 *                                          Some helper functions
 *=======================================================================================================*/

static bool read_le(parser* p, unsigned long* u) {
    return (8 == pread(u, 8, p));
}

static void check_bitstream(parser* p) {
    if (p->bits_avail == 0) {
        if (!read_le(p, &(p->bits))) printf("Unexpected EOF\n");
        p->bits_avail = sizeof(unsigned long) * 8;
    }
}

static bool read_bit2(parser* p) {
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
}

static bool read_bit1(parser* p) {
    check_bitstream(p);
    unsigned long bp = p->bits & 0x01;
    p->bits >>= 1;
    p->bits_avail -= 1;
    return bp;
}

// determine file type
bool matches(const char* ext, char format, char comp) {
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
void file_type(const char* pathname, char* type)
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

/*=======================================================================================================
 *                                  parser related funcitons
 *=======================================================================================================*/

void init_file_reader(file_reader* fr, const char* filename, char comp)
{
    //
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
        if (out_terminal[0] == 6) {
            out_terminal[0] = 5;
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


/*=======================================================================================================
 *                                  RexBDD related funcitons
 *=======================================================================================================*/

// unmark all nodes in the forest (initializing for counting the marked nodes)
void unmark_forest(
                rexdd_forest_t *F)
{
    uint_fast64_t p, n;
    for (p=0; p<F->M->pages_size; p++) {
        const rexdd_nodepage_t *page = F->M->pages+p;
        for (n=0; n<page->first_unalloc; n++) {
            if (rexdd_is_packed_marked(page->chunk+n)) {
                rexdd_unmark_packed(page->chunk+n);
            }
        } // for n
    } // for p
}

// mark the nonterminal nodes from root in the forest *F. This is used for counting the number of nodes
void mark_nodes(
                rexdd_forest_t *F, 
                rexdd_node_handle_t root)
{
    if (!rexdd_is_terminal(root)) {
        if (!rexdd_is_packed_marked(rexdd_get_packed_for_handle(F->M, root))) {
            rexdd_mark_packed(rexdd_get_packed_for_handle(F->M,root));
        }
        rexdd_node_handle_t low, high;
        low = rexdd_unpack_low_child(rexdd_get_packed_for_handle(F->M, root));
        high = rexdd_unpack_high_child(rexdd_get_packed_for_handle(F->M, root));
        mark_nodes(F, low);
        mark_nodes(F, high);
    }
    
}
// build a forest recording the input function (2D array of bool with specific container number)
rexdd_edge_t union_minterm(rexdd_forest_t* F, rexdd_edge_t* root, char* minterm, uint32_t K)
{
    // printf("Here is union minterm at level %u\n",K);

    // the final answer
    rexdd_edge_t ans;
    ans.label.rule = rexdd_rule_X;
    ans.label.complemented = 0;
    ans.label.swapped = 0;

    // terminal case 
    if (K==0) {
        // 
        rexdd_set_edge(&ans,
                        rexdd_rule_X,
                        1,
                        0,
                        rexdd_make_terminal(0));
        return ans;
    }

    /* first check if this minterm can be captured by root edge
     *      if so, good! we can return root edge for this;
     *      if not, OK! let's build a new node
     */
    bool minterm_bol[K+2];
    minterm_bol[0] = 0;
    for (uint32_t i = 1; i<=K; i++) {
        minterm_bol[i] = (minterm[i] == '1') ? 1 : 0;
    }
    minterm[K+1] = 0;
    // printf("first check eval\n");
    if (rexdd_eval(F,root,K,minterm_bol)) {
        rexdd_set_edge(&ans,
                        root->label.rule,
                        root->label.complemented,
                        root->label.swapped,
                        root->target);
        return ans;
    }


    /*
     * here means the root edge can not capture the minterm!
     *      we need to break root edge if it's a long edge
     */
    uint32_t root_skip;     // if the root edge is a long edge or not
    if (rexdd_is_terminal(root->target)) {
        root_skip = K;
    } else {
        root_skip = K - rexdd_unpack_level(rexdd_get_packed_for_handle(F->M, root->target));
    }

    // determine down pointers
    rexdd_edge_t root_down0, root_down1;
    if (root_skip == 0) {
        // root edge does not skip ndoes at this level
        rexdd_edge_label_t l;
        rexdd_unpack_low_edge(rexdd_get_packed_for_handle(F->M, root->target), &l);
        rexdd_set_edge(&root_down0,
                        l.rule ,
                        l.complemented,
                        l.swapped,
                        rexdd_unpack_low_child(rexdd_get_packed_for_handle(F->M, root->target)));
        rexdd_unpack_high_edge(rexdd_get_packed_for_handle(F->M, root->target), &l);
        rexdd_set_edge(&root_down1,
                        l.rule ,
                        l.complemented,
                        l.swapped,
                        rexdd_unpack_high_child(rexdd_get_packed_for_handle(F->M, root->target)));
    } else {
        assert(root_skip > 0);
        // here means the root edge is a long edge (X, EL, EH, AL, AH)
        if (root->label.rule == rexdd_rule_X) {
            // this is easy
            rexdd_set_edge(&root_down0,
                            root->label.rule,
                            root->label.complemented,
                            root->label.swapped,
                            root->target);
            rexdd_set_edge(&root_down1,
                            root->label.rule,
                            root->label.complemented,
                            root->label.swapped,
                            root->target);
        } else if (rexdd_is_EL(root->label.rule)) {
            // root edge is EL
            rexdd_set_edge(&root_down0,
                            rexdd_rule_X,
                            rexdd_is_one(root->label.rule),
                            0,
                            rexdd_make_terminal(0));
            rexdd_set_edge(&root_down1,
                            (root_skip == 1) ? rexdd_rule_X : root->label.rule,
                            root->label.complemented,
                            root->label.swapped,
                            root->target);
        } else if (rexdd_is_EH(root->label.rule)) {
            // root edge is EH
            rexdd_set_edge(&root_down0,
                            (root_skip == 1) ? rexdd_rule_X : root->label.rule,
                            root->label.complemented,
                            root->label.swapped,
                            root->target);
            rexdd_set_edge(&root_down1,
                            rexdd_rule_X,
                            rexdd_is_one(root->label.rule),
                            0,
                            rexdd_make_terminal(0));
        } else if (rexdd_is_AL(root->label.rule)) {
            // root edge is AL
            rexdd_set_edge(&root_down0,
                            root->label.rule,
                            root->label.complemented,
                            root->label.swapped,
                            root->target);
            rexdd_set_edge(&root_down1,
                            rexdd_rule_X,
                            root->label.complemented,
                            root->label.swapped,
                            root->target);
            if (root_skip == 2) {
                // AL edge skips >= 2 nodes
                root_down0.label.rule = (rexdd_is_one(root->label.rule)) ? rexdd_rule_EL1 : rexdd_rule_EL0;
            }
        } else if (rexdd_is_AH(root->label.rule)) {
            // root edge is AH
            rexdd_set_edge(&root_down0,
                            rexdd_rule_X,
                            root->label.complemented,
                            root->label.swapped,
                            root->target);
            rexdd_set_edge(&root_down1,
                            root->label.rule,
                            root->label.complemented,
                            root->label.swapped,
                            root->target);
            if (root_skip == 2) {
                // AH edge skips >= 2 nodes
                root_down1.label.rule = (rexdd_is_one(root->label.rule)) ? rexdd_rule_EH1 : rexdd_rule_EH0;
            }
        }
    }


    // Build new node
    rexdd_unpacked_node_t tmp;
    tmp.level = K;

    if (minterm[K] == '1') {
        tmp.edge[1] = union_minterm(F, &root_down1, minterm, K-1);
        tmp.edge[0] = root_down0;
    } else {
        tmp.edge[0] = union_minterm(F, &root_down0, minterm, K-1);
        tmp.edge[1] = root_down1;
    }

    rexdd_reduce_edge(F, K, ans.label, tmp, &ans);

    // if (K == 25) {
    //     printf("the union edge's target is %llu\n", ans.target);
    // }

    return ans;

}




/*=======================================================================================================
 *                                           main function here
 =======================================================================================================*/


int main(int argc, const char* const* argv)
{
    //
    if (argc == 1) {
        printf("Need one input file\n");
        return 0;
    }

    rexdd_forest_t F;
    rexdd_forest_settings_t s;
    rexdd_edge_t root_edge[5*(argc - 1)];
    // Initializing the root edges
    for (int i=0; i<5*(argc-1); i++) {
        rexdd_set_edge(&root_edge[i], rexdd_rule_X, 0, 0, rexdd_make_terminal(0));
    }


    const char* infile = 0;

    for (int n=1; n<argc; n++) {

        infile = argv[n];

        // now let's read minterms
        char fmt = 0 , comp = 0;
        char type[2];
        file_type(infile, type);
        fmt = type[0];
        comp = type[1];

        file_reader fr;
        init_file_reader(&fr, infile, comp);
        parser p;
        init_parser(&p, &fr);
        switch (fmt) {
            case 'p':
            case 'P':
                read_header_pla(&p);
                break;        
            case 'b':
            case 'B':
                read_header_bin(&p);
                break;
            default:
                printf("No parser for format %c\n",fmt);
                // return 0;
        }

        printf("Building forest %d for %s\n", n, infile);
        printf("\tThe format is %c\n", fmt);
        printf("\tThe compress is %c\n", comp);
        printf("\tThe number of inbits is %u\n", p.inbits);
        printf("\tThe number of outbits is %u\n", p.outbits);
        printf("\tThe number of minterms is %lu\n", p.numf);

        char term;
        unsigned num_inputbits = p.inbits;
        char inputbits[num_inputbits + 2];          // the first and last is 0; index is the level
        bool inputbits_bol[num_inputbits + 2];

        // Initializing the forest
        if (n == 1) {
            rexdd_default_forest_settings(p.inbits, &s);
            rexdd_init_forest(&F, &s);
        }

        int index = 0;
        // rexdd_edge_t union_edge;
        for (;;) {
            if (fmt == 'p') {
                if (!read_minterm_pla(&p, inputbits, &term)) break;
            } else {
                if (!read_minterm_bin(&p, inputbits, &term)) break;
            }
            // reverse(inputbits, p.inbits);
            index = term - '1' + 5*(n-1);
            root_edge[index] = union_minterm(&F, &root_edge[index], inputbits, p.inbits);

            // gc TBD here for huge number of nodes

        }
        printf("Done building!\n");

        printf("Evaling...\n");
        for (;;) {
            if (fmt == 'p') {
                if (!read_minterm_pla(&p, inputbits, &term)) break;
            } else {
                if (!read_minterm_bin(&p, inputbits, &term)) break;
            }
            index = term - '1' + 5*(n-1);

            for (unsigned i=0; i< p.inbits; i++){
                if (inputbits[i] == '1') {
                    inputbits_bol[i] = 1;
                } else {
                    inputbits_bol[i] = 0;
                }
            }
            if (!rexdd_eval(&F, &root_edge[index], p.inbits, inputbits_bol)) {
                printf("eval test fail!\n");
                return 1;
            }
            
        }
        printf("Evaluation pass!\n\n");
        free_parser(&p); // file reader will be free
    }

    printf("Unmarking the forest...\n");
    unmark_forest(&F);
    printf("Done unmarking!\n");


    printf("Marking nonterminal nodes in use from roots...\n");
    for (int i=0; i<5*(argc-1); i++) {
        if (rexdd_is_terminal(root_edge[i].target)) {
            printf("\troot %d : %d is T0\n",(i/5)+1, i%5);
        } else {
            printf("\troot %d : %d is %llu\n",(i/5)+1, i%5, root_edge[i].target);
        }
        mark_nodes(&F, root_edge[i].target);
    }
    printf("Done marking!\n");


    printf("Counting the total number of nodes in forest...\n");
    uint_fast64_t q, n;
    uint64_t num_nodes = 0;
    for (q=0; q<F.M->pages_size; q++) {
        const rexdd_nodepage_t *page = F.M->pages+q;
        for (n=0; n<page->first_unalloc; n++) {
            if (rexdd_is_packed_marked(page->chunk+n)) {
                num_nodes++;
            }
        } // for n
    } // for p
    if (argc == 2) {
        printf("Total number of nodes in %s is %llu\n", infile, num_nodes);
    } else {
        printf("Total number of nodes in forest is %llu\n", num_nodes);
    }


    rexdd_free_forest(&F);

}