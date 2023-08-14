#include "forest.h"
#include "helpers.h"
#include "operation.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

long seed =999;

/* Random function generating value between 0 and 1 */
double Random()
{
  const long MODULUS = 2147483647L;
  const long MULTIPLIER = 48271L;
  const long Q = MODULUS / MULTIPLIER;
  const long R = MODULUS % MULTIPLIER;

  long t = MULTIPLIER * (seed % Q) - R * (seed / Q);
  if (t > 0) {
    seed = t;
  } else {
    seed = t + MODULUS;
  }
  return ((double) seed / MODULUS);
}

void decimalToBinary(long long decimal, bool* binary)
{
    int index = 0;
    while (decimal > 0) {
        binary[index++] = decimal % 2;
        decimal = decimal / 2;
    }
}

void show_edge(rexdd_edge_t ans)
{
    char rule[16];
    for (int i=0; i<16; i++) {
      rule[i] = 0;
    }
    if (ans.label.rule == rexdd_rule_X) {
      strcpy(rule, "X");
    } else if (ans.label.rule == rexdd_rule_EL0) {
      strcpy(rule, "EL0");
    } else if (ans.label.rule == rexdd_rule_EL1) {
      strcpy(rule, "EL1");
    } else if (ans.label.rule == rexdd_rule_EH0) {
      strcpy(rule, "EH0");
    } else if (ans.label.rule == rexdd_rule_EH1) {
      strcpy(rule, "EH1");
    } else if (ans.label.rule == rexdd_rule_AL0) {
      strcpy(rule, "AL0");
    } else if (ans.label.rule == rexdd_rule_AL1) {
      strcpy(rule, "AL1");
    } else if (ans.label.rule == rexdd_rule_AH0) {
      strcpy(rule, "AH0");
    } else if (ans.label.rule == rexdd_rule_AH1) {
      strcpy(rule, "AH1");
    } 
    printf("\trule is %s\n", rule);
    printf("\tcomp is %d\n", ans.label.complemented);
    printf("\tswap is %d\n", ans.label.swapped);
    printf("\ttarget is %s%llu\t", rexdd_is_terminal(ans.target)?"T":"", rexdd_is_terminal(ans.target)? rexdd_terminal_value(ans.target): ans.target);
}

void showCommon(char* f1, char* f2, long long n)
{
    long long count = 0;
    for (long long i=0; i<n; i++) {
        count += (f1[i]==f2[i]);
    }
    printf("number of common minterms is %lld\n", count);
}

long long card_edge(rexdd_forest_t* F, rexdd_edge_t* root, uint32_t lvl)
{
    if (lvl == 0) {
        assert(root->label.rule == rexdd_rule_X);
        return rexdd_terminal_value(root->target) ^ root->label.complemented;
    }
    // determine down pointers
    rexdd_edge_t root_down0, root_down1;
    root_down0 = rexdd_expand_childEdge(F, lvl, root, 0);
    root_down1 = rexdd_expand_childEdge(F, lvl, root, 1);
    long long ans0 = 0, ans1 = 0;
    ans0 = card_edge(F, &root_down0, lvl-1);
    ans1 = card_edge(F, &root_down1, lvl-1);
    return ans0 + ans1;
}

int main(int argc, const char* const* argv)
{
    if (argc == 0) {
		printf("Usage: ./check_and [num_val]\n");
		printf("\tThis will randomly generate two boolean functions to test logic operation\n");
	}
    uint32_t num_vals = atoi(argv[1]);

    rexdd_forest_t F;
    rexdd_forest_settings_t s;
    rexdd_default_forest_settings(num_vals, &s);
    rexdd_init_forest(&F, &s);

    // Randomly generate minterms and build BDD1 and BDD2
    rexdd_edge_t e1, e2, ans;
    rexdd_set_edge(&e1,
                    rexdd_rule_X,
                    0,
                    0,
                    rexdd_make_terminal(0));
    rexdd_set_edge(&e2,
                    rexdd_rule_X,
                    0,
                    0,
                    rexdd_make_terminal(0));

    long long rows = 0x01LL<<(num_vals);
    char* function1 = malloc(rows*sizeof(char));
    char* function2 = malloc(rows*sizeof(char));
    long long count1 = 0, count2 = 0, count = 0;
    // set functions
    for (long long i=0; i<rows; i++) {
        function1[i] = (Random()>0.5)?1:0;
        function2[i] = (Random()>0.5)?1:0;
        if (function1[i]) count1++;
        if (function2[i]) count2++;
        if (function1[i]&&function2[i]) count++;
    }
    functionToEdge(&F, function1, &e1, num_vals, 0, rows-1);
    functionToEdge(&F, function2, &e2, num_vals, 0, rows-1);

    printf("e1 is:\n");
    show_edge(e1);
    printf("level is %d\n", (rexdd_is_terminal(e1.target))?0:rexdd_unpack_level(rexdd_get_packed_for_handle(F.M, e1.target)));
    printf("e2 is:\n");
    show_edge(e2);
    printf("level is %d\n", (rexdd_is_terminal(e2.target))?0:rexdd_unpack_level(rexdd_get_packed_for_handle(F.M, e2.target)));

    // Call rexdd_AND_edges, and check results
    ans = rexdd_AND_edges(&F, &e1, &e2, num_vals);
    printf("ans is:\n");
    show_edge(ans);
    printf("level is %d\n", (rexdd_is_terminal(ans.target))?0:rexdd_unpack_level(rexdd_get_packed_for_handle(F.M, ans.target)));

    printf("number of minterms(1) for e1:\t%lld\n",count1);
    printf("number of minterms(1) for e2:\t%lld\n",count2);
    printf("number of common minterms:\t%lld\n",count);
    printf("cardinality of e1:\t%lld\n", card_edge(&F, &e1, num_vals));
    printf("cardinality of e2:\t%lld\n", card_edge(&F, &e2, num_vals));
    
    long long card_ans = 0;
    card_ans = card_edge(&F, &ans, num_vals);
    printf("cardinality of AND ans:\t%lld\n", card_ans);    
    if (count == card_ans){
        printf("AND cardinality test PASS!\n");
    } else {
        // FILE* fout;
        // rexdd_edge_t err1, err2, errA;
        // rexdd_set_edge(&err1,
        //                 rexdd_rule_X,
        //                 1,
        //                 0,
        //                 496);
        // rexdd_set_edge(&err2,
        //                 rexdd_rule_X,
        //                 0,
        //                 0,
        //                 997);
        // rexdd_set_edge(&errA,
        //                 rexdd_rule_X,
        //                 0,
        //                 0,
        //                 1461);
        // fout = fopen("and_e1.gv", "w+");
        // build_gv(fout, &F, err1);
        // fclose(fout);
        // fout = fopen("and_e2.gv", "w+");
        // build_gv(fout, &F, err2);
        // fclose(fout);
        // fout = fopen("and_ans.gv", "w+");
        // build_gv(fout, &F, errA);
        // fclose(fout);
        printf("AND cardinality test ERROR!\n");
    }
    rexdd_edge_t or_ans, xor_ans;
    or_ans = rexdd_OR_edges(&F, &e1, &e2, num_vals);
    long long card_or, card_xor;
    card_or = card_edge(&F, &or_ans, num_vals);
    printf("\ncardinality of OR ans:\t%lld\n", card_or);
    if (count1 + count2 - card_ans == card_or) {
        printf("OR cardinality test PASS!\n");
    } else {
        printf("OR cardinality test ERROR!\n");
    }
    xor_ans = rexdd_XOR_edges(&F, &e1, &e2, num_vals);
    card_xor = card_edge(&F, &xor_ans, num_vals);
    printf("\ncardinality of XOR ans:\t%lld\n", card_xor);

    bool minterm[num_vals];
    bool minterm_eval[num_vals+2];
    minterm_eval[0] = 0;
    minterm_eval[num_vals+1] = 0;
    for (uint32_t m=0; m<num_vals; m++) minterm[m] = 0;

    for (long long n=0; n<rows; n++) {
        decimalToBinary(n, minterm);
        for (uint32_t m=1; m<num_vals+1; m++) {
            minterm_eval[m] = minterm[m-1];
        }
        if (function1[n] != rexdd_eval(&F, &e1, num_vals, minterm_eval)){
            printf("e1 evaluation error!\n");
            exit(1);
        }
        if (function2[n] != rexdd_eval(&F, &e2, num_vals, minterm_eval)){
            printf("e2 evaluation error!\n");
            exit(1);
        }
        bool ans_eval = rexdd_eval(&F, &ans, num_vals, minterm_eval);
        if ((function1[n] && function2[n]) != ans_eval){
            printf("\nAND_ans evaluation error!\n");
            for (uint32_t m=0; m<num_vals; m++) {
                printf("%d ", minterm[m]);
            }
            printf("\tf1: %d\tf2: %d\n", function1[n], function2[n]);
            exit(1);
        }
        ans_eval = rexdd_eval(&F, &or_ans, num_vals, minterm_eval);
        if ((function1[n] || function2[n]) != ans_eval){
            printf("\nOR_ans evaluation error!\n");
            for (uint32_t m=0; m<num_vals; m++) {
                printf("%d ", minterm[m]);
            }
            printf("\tf1: %d\tf2: %d\n", function1[n], function2[n]);
            exit(1);
        }
        ans_eval = rexdd_eval(&F, &xor_ans, num_vals, minterm_eval);
        if ((function1[n] ^ function2[n]) != ans_eval){
            printf("\nXOR_ans evaluation error!\n");
            for (uint32_t m=0; m<num_vals; m++) {
                printf("%d ", minterm[m]);
            }
            printf("\tf1: %d\tf2: %d\n", function1[n], function2[n]);
            exit(1);
        }
    }
    printf("\n");
    printf("AND evaluation test PASS!\n");
    printf("OR evaluation test PASS!\n");
    printf("XOR evaluation test PASS!\n");

    rexdd_free_forest(&F);
    free(function1);
    free(function2);
    return 0;
}