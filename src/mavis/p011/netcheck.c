/* netcheck.c -- independent verifier for a sorting network.
 *
 * Reads comparators (one "a b" pair per line, 0-based channel indices) and
 * reports how many of the 2^n binary inputs are NOT sorted.
 *
 * Bit-sliced: 64 inputs at a time.  The bit patterns of the low 6 channels
 * are swept inside a word; the higher channels are constant within a group
 * of 64, so each group of 64 inputs needs one pass over the comparators.
 *
 * This is deliberately an independent implementation of the semantics -- it
 * shares no code with the SAT encoder, so it doubles as a cross-check.
 *
 * Usage: netcheck <comparators.txt> [n]
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

static uint64_t wire[64];

int main(int argc, char **argv)
{
    if (argc < 2) { fprintf(stderr, "usage: %s <comparators.txt> [n]\n", argv[0]); return 1; }
    int n = (argc > 2) ? atoi(argv[2]) : 17;
    if (n < 2 || n > 32) { fprintf(stderr, "bad n\n"); return 2; }
    long TOTAL = 1L << n;

    int c[4096][2], nc = 0;
    FILE *f = fopen(argv[1], "r");
    if (!f) { perror(argv[1]); return 1; }
    int a, b;
    while (nc < 4096 && fscanf(f, "%d %d", &a, &b) == 2) {
        if (a < 0 || b < 0 || a >= n || b >= n) { fprintf(stderr, "channel out of range\n"); return 2; }
        c[nc][0] = a; c[nc][1] = b; nc++;
    }
    fclose(f);

    long bad = 0, firstbad = -1;
    uint64_t pats[64];
    for (long base = 0; base < TOTAL; base += 64) {
        for (int k = 0; k < 64; k++) pats[k] = (uint64_t)(base + k);
        for (int i = 0; i < n; i++) {
            uint64_t w = 0;
            if (i < 6) { for (int k = 0; k < 64; k++) if ((pats[k] >> i) & 1ULL) w |= 1ULL << k; }
            else { if ((base >> i) & 1ULL) w = ~0ULL; else w = 0ULL; }
            wire[i] = w;
        }
        for (int t = 0; t < nc; t++) {
            uint64_t x = wire[c[t][0]], y = wire[c[t][1]];
            wire[c[t][0]] = x & y;
            wire[c[t][1]] = x | y;
        }
        uint64_t uns = 0;
        for (int i = 0; i + 1 < n; i++) uns |= wire[i] & ~wire[i + 1];
        int nb = __builtin_popcountll(uns);
        if (nb) {
            bad += nb;
            if (firstbad < 0) firstbad = base + __builtin_ctzll(uns);
        }
    }
    printf("comparators=%d unsorted_inputs=%ld first_counterexample=%ld\n", nc, bad, firstbad);
    return bad == 0 ? 0 : 1;
}
