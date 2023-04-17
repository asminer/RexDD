#include "forest.h"
#include "helpers.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/*======================================================
 *  The seed for Bernoulli sampling
 =====================================================*/
long seed = 32;

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

/*======================================================
 *  helper function to sort
 =====================================================*/
void swap(uint64_t *a, uint64_t *b)
{
    uint64_t temp = *a;
    *a = *b;
    *b = temp;
}

long long partition(uint64_t arr[], long long low, long long high)
{
    uint64_t pivot = arr[high];
    long long i = low - 1;

    for (long long j = low; j <= high - 1; j++) {
        if (arr[j] < pivot) {
            i++;
            swap(&arr[i], &arr[j]);
        }
    }
    swap(&arr[i + 1], &arr[high]);
    return i + 1;
}

void quicksort(uint64_t arr[], long long low, long long high)
{
    if (low < high) {
        long long pi = partition(arr, low, high);

        quicksort(arr, low, pi - 1);
        quicksort(arr, pi + 1, high);
    }
}

/*======================================================
 *  helper function for main process
 =====================================================*/
void functionToEdge(rexdd_forest_t* F, char* functions, rexdd_edge_t* root_out, int L, unsigned long start, unsigned long end)
{
    //
    rexdd_unpacked_node_t temp;
    temp.level = L;
    temp.edge[0].label.rule = rexdd_rule_X;
    temp.edge[1].label.rule = rexdd_rule_X;
    temp.edge[0].label.swapped = 0;
    temp.edge[1].label.swapped = 0;
    temp.edge[0].label.complemented = 0;
    temp.edge[1].label.complemented = 0;
    rexdd_edge_label_t l;
    l.rule = rexdd_rule_X;
    l.complemented = 0;
    l.swapped = 0;
    /*  terminal case */
    if ((end-start == 1) && (L==1)) {
        // printf("\tterminal case!\n");
        temp.edge[0].target = rexdd_make_terminal(functions[start]);
        temp.edge[1].target = rexdd_make_terminal(functions[end]);
        
        rexdd_reduce_edge(F, L, l, temp, root_out);
        return;
    }
    const int next_lvl = L-1;
    // printf("\t\tgo level %d!\n", next_lvl);
    functionToEdge(F, functions, &temp.edge[0], next_lvl, start, (end-start)/2+start);
    functionToEdge(F, functions, &temp.edge[1], next_lvl, (end-start)/2+start+1, end);
    // printf("\t\treduce level %d!\n", temp.level);
    rexdd_reduce_edge(F, L, l, temp, root_out);
}

uint64_t count_nodes(rexdd_forest_t* F, rexdd_edge_t* root)
{
    unmark_forest(F);
    mark_nodes(F, root->target);
    uint_fast64_t q, n;
    uint64_t num_nodes = 0;
    for (q=0; q<F->M->pages_size; q++) {
        const rexdd_nodepage_t *page = F->M->pages+q;
        for (n=0; n<page->first_unalloc; n++) {
            if (rexdd_is_packed_marked(page->chunk+n)) {
                num_nodes++;
            }
        } // for n
    } // for p
    return num_nodes;
}

/*======================================================
 *          main function
 =====================================================*/
int main(int argc, const char* const* argv)
{
    if (argc!=5) {
        printf("Usage: ./sampling (option) (#functions) (#variables) (probability)\n");
        printf("\tOption:\n");
        printf("\t\t -s: show details;\n");
        printf("\t\t -d: don't care the details, show me the density\n");
        printf("\t\t -c: don't care the details, show me the cumulative distribution\n");
        return 0;
    }

    /*  Read number of functions and variables  */
    char option = 0;
    if (argv[1][1] == 's') {
        option = 1;
    } else if (argv[1][1] == 'd') {
        option = 2;
    } else if (argv[1][1] == 'c') {
        option = 3;
    } else {
        printf("Unknown option!\n");
        return 0;
    }
    long long num_function = strtoll(argv[2], 0, 10);
    int num_vars = atoi(argv[3]);
    double p = strtod(argv[4], 0);
    printf("\t+ Number of function is %lld\n", num_function);
    printf("\t+ Number of variables is %d\n", num_vars);
    printf("\t+ The Probability setting bit to 1 is %2f\n", p);

    /*  Time    */
    clock_t start_time, end_time;

    /*  Initialize the array of bits for function   */
    long long rows = 0x01LL<<(num_vars);
    rexdd_edge_t root;
    printf("mallcing and initializing for function bits......\n");
    start_time = clock();
    char *function = malloc(rows*sizeof(char));
    if (!function){
        printf("Unable to malloc for function\n");
        exit(1);
    }
    for (long long i=0; i<rows; i++) {
        function[i] = 0;
    }
    /*  Counted numbers of nodes    */
    // initialize as needed
    uint64_t* numbers = 0;
    if (option > 1) {
        numbers = malloc(num_function*sizeof(uint64_t));
        if (!numbers){
            printf("Unable to malloc for numbers\n");
            exit(1);
        }
        for (long long i=0; i<num_function; i++) {
            numbers[i] = 0;
        }
    }
    end_time = clock();
    printf("\033[1A"); // Move cursor up one line
    printf("\033[K"); // Clear line
    printf("** malloc amd initializing time used %f seconds\n", (double)(end_time-start_time)/CLOCKS_PER_SEC);

    /*  Initialize the forest   */
    rexdd_forest_t F;
    rexdd_forest_settings_t s;
    u_int64_t num_nodes = 0;
    rexdd_default_forest_settings(num_vars, &s);

    /*  Start   */
    start_time = clock();
    for (long long i=0; i<num_function; i++) {
        // initialize forest for this function
        rexdd_init_forest(&F, &s);
        /*  set this function   */
        for (long long j=0; j<rows; j++) {
            function[j] = (Random()>p)?1:0;
        }
        functionToEdge(&F, function, &root, num_vars, 0, rows-1);
        num_nodes = count_nodes(&F, &root);
        if (option > 1) {
            numbers[i] = num_nodes;
            printf("Got function root edges [%lld / %lld]\n", i+1, num_function);
            if (i<num_function-1) {
                printf("\033[1A"); // Move cursor up one line
                printf("\033[K"); // Clear line
            }
        } else {
            printf("%s number of nodes over function %lld is %llu\n", TYPE, i+1, num_nodes);
        }
        // free forest for this function
        rexdd_free_forest(&F);
    } // end for loop of functions
    end_time = clock();
    printf("** building and counting time: %f seconds\n", (double)(end_time-start_time)/CLOCKS_PER_SEC);

    FILE* fout = 0;    
    if (option > 1) {
        start_time = clock();
        quicksort(numbers, 0, num_function-1);
        end_time = clock();
        printf("** quick sort time: %f seconds\n", (double)(end_time-start_time)/CLOCKS_PER_SEC);
        printf("=================| Results |===============\n");
        if (option == 2) {
            fout = fopen("sampling_density.txt", "w+");
        } else {
            fout = fopen("sampling_cumulative.txt", "w+");
        }
        printf("#node(%s)\t\t#count(%%)\n", TYPE);
        fprintf(fout, "#node(%s)\t\t#count(%%)\n", TYPE);
        uint64_t pre = numbers[0];
        long long count = 0;
        for (long long n=0; n<num_function; n++) {
            if (numbers[n] == pre){
                count++;
                continue;
            }
            printf("%llu\t\t\t%0.4f\n", pre, (double)100*count/num_function);
            fprintf(fout, "%llu\t\t\t%0.4f\n", pre, (double)100*count/num_function);
            if (option == 2) {
                count = 1;
            } else if (option == 3) {
                count++;
            }
            pre = numbers[n];
        }
        printf("%llu\t\t\t%0.4f\n", pre, (double)100*count/num_function);
        fprintf(fout, "%llu\t\t\t%0.4f\n", pre, (double)100*count/num_function);

    }
    /*  Free every thing    */
    free(function);
    if (option > 1) {
        free(numbers);
        fclose(fout);
    }
    return 0;
}