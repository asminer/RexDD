

#include <stdio.h>

const unsigned small_primes[] = { 2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71, 73, 79, 83, 89, 97, 0 };

char is_prime(unsigned long p)
{
    // Check the small primes first, that will rule out lots of cases
    unsigned i;
    unsigned f;
    for (i=0; small_primes[i]; ++i) {
        f = small_primes[i];
        if (0 == p % f) return 0;
    }
    for (f+=2; f*f <= p; f+=2)
    {
        if (0== p % f) return 0;
    }
    return 1;
}

unsigned long twoNmodp(unsigned n, unsigned long p)
{
    // Safe for p < 2^63
    unsigned long x = 1;
    for (; n; --n)
    {
        x = (2 * x) % p;
    }
    return x;
}

void showmult(unsigned n, unsigned long p)
{
    const unsigned long a = twoNmodp(n, p);
    const unsigned long q = p / a;
    const unsigned long r = p % a;


    printf("%3u  %16lu  %16lu  %16lu  %s\n",
        n, a, q, r, r<q ? " OK ": "nope");
}

int main()
{
    unsigned long p;
    printf("Enter modulus p: ");
    scanf("%lu", &p);
    printf("\n");


    printf("P %lu is %s\n", p, is_prime(p) ? "prime" : "composite");


    printf(" n    a  = 2^n mod p         p / a             p %% a           \n");
    printf("---  ----------------  ----------------  ----------------  ----\n");

    showmult(46, p);
    showmult(78, p);

    return 0;
}
