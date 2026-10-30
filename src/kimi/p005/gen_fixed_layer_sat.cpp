#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <set>
#include <string>
#include <vector>
using namespace std;

static const int N = 6561, PN = 8192, TRUE_W = INT32_MAX, FALSE_W = 0;
static uint8_t hi[N], lo[N], third4[81][81];
static FILE *out;
static int nextvar;
static long long clauses = 0, cap3 = 0, cap2 = 0, comparators = 0;

static inline int third(int a, int b) {
    return third4[hi[a]][hi[b]] * 81 + third4[lo[a]][lo[b]];
}
static void emit(initializer_list<int> literals) {
    for (int lit : literals) fprintf(out, "%d ", lit);
    fputs("0\n", out);
    clauses++;
}
static void comparator(int a, int b, int &high, int &low) {
    comparators++;
    if (a == TRUE_W || b == TRUE_W) {
        high = TRUE_W;
        low = (a == TRUE_W ? b : a);
        return;
    }
    if (a == FALSE_W) { high = b; low = FALSE_W; return; }
    if (b == FALSE_W) { high = a; low = FALSE_W; return; }
    if (a == b) { high = low = a; return; }
    int mx = nextvar++, mn = nextvar++;
    high = mx; low = mn;
    emit({-a, mx}); emit({-b, mx}); emit({a, b, -mx});
    emit({-mn, a}); emit({-mn, b}); emit({mn, -a, -b});
}
static int coordinate(int x, int coord) {
    static const int pow3[8] = {2187, 729, 243, 81, 27, 9, 3, 1};
    return (x / pow3[coord]) % 3;
}
static set<int> read_points(const string &path) {
    ifstream in(path);
    if (!in) { cerr << "cannot open point file: " << path << "\n"; exit(2); }
    set<int> points;
    string token;
    while (in >> token) {
        if (!token.empty() && token[0] == '#') { getline(in, token); continue; }
        int x = stoi(token);
        if (x < 0 || x >= N || !points.insert(x).second) {
            cerr << "invalid or duplicate point: " << x << "\n"; exit(3);
        }
    }
    return points;
}
static void check_cap(const set<int> &points) {
    for (int a : points) for (int b : points) if (a < b) {
        int c = third(a, b);
        if (points.count(c)) {
            cerr << "input cap conflict: " << a << " " << b << " " << c << "\n";
            exit(3);
        }
    }
}

int main(int argc, char **argv) {
    if (argc < 6 || argc > 9) {
        fprintf(stderr, "usage: gen_fixed_layer_sat out.cnf full_cap.txt coordinate value target [map.txt|-] [witness_cap.txt|-] [balance_free_layers]\n");
        return 2;
    }
    string cap_path = argv[2], map_path = (argc >= 7 && string(argv[6]) != "-") ? argv[6] : "";
    bool balance = argc >= 9 && atoi(argv[8]);
    int coord = atoi(argv[3]), value = atoi(argv[4]), target = atoi(argv[5]);
    if (coord < 0 || coord >= 8 || value < 0 || value >= 3) {
        fprintf(stderr, "invalid coordinate or layer value\n"); return 2;
    }
    set<int> full_cap = read_points(cap_path), fixed;
    check_cap(full_cap);
    for (int x : full_cap) if (coordinate(x, coord) == value) fixed.insert(x);
    if (fixed.empty() || (int)fixed.size() >= target) {
        fprintf(stderr, "invalid fixed layer size %zu for target %d\n", fixed.size(), target);
        return 3;
    }
    for (int x = 0; x < N; x++) { hi[x] = x / 81; lo[x] = x % 81; }
    for (int a = 0; a < 81; a++) for (int b = 0; b < 81; b++) {
        int aa = a, bb = b, w = 0, place = 1;
        for (int i = 0; i < 4; i++) {
            w += ((3 - (aa % 3 + bb % 3) % 3) % 3) * place;
            aa /= 3; bb /= 3; place *= 3;
        }
        third4[a][b] = w;
    }
    vector<int> variable(N, FALSE_W), point_of_var(1, -1);
    for (int x = 0; x < N; x++) if (coordinate(x, coord) != value) {
        variable[x] = point_of_var.size();
        point_of_var.push_back(x);
    }
    int originals = point_of_var.size() - 1;
    int free_target = target - fixed.size();
    if (free_target < 0 || free_target > originals) {
        fprintf(stderr, "impossible free target %d with %d variables\n", free_target, originals);
        return 3;
    }
    if (!map_path.empty()) {
        ofstream map(map_path);
        if (!map) { cerr << "cannot write map: " << map_path << "\n"; return 2; }
        map << "# variable point\n";
        for (int v = 1; v <= originals; v++) map << v << " " << point_of_var[v] << "\n";
    }
    out = fopen(argv[1], "w");
    if (!out) { perror(argv[1]); return 2; }
    fprintf(out, "p cnf %010d %011lld\n", 0, 0LL);
    nextvar = originals + 1;
    long long lines = 0;
    for (int a = 0; a < N; a++) for (int b = a + 1; b < N; b++) {
        int c = third(a, b);
        if (c <= b) continue;
        lines++;
        int la = coordinate(a, coord), lb = coordinate(b, coord), lc = coordinate(c, coord);
        if (la == lb && lb == lc) {
            if (la == value) {
                if (fixed.count(a) && fixed.count(b) && fixed.count(c)) {
                    fprintf(stderr, "fixed layer cap conflict\n"); return 4;
                }
                continue;
            }
            emit({-variable[a], -variable[b], -variable[c]}); cap3++;
        } else {
            if (la + lb + lc != 3) {
                fprintf(stderr, "bad layer line %d %d %d values %d %d %d\n", a, b, c, la, lb, lc);
                return 4;
            }
            int f = la == value ? a : (lb == value ? b : c);
            if (!fixed.count(f)) continue;
            vector<int> lits;
            for (int x : {a, b, c}) if (x != f) lits.push_back(-variable[x]);
            if (lits.size() != 2 || lits[0] == 0 || lits[1] == 0) {
                fprintf(stderr, "bad cross-layer clause\n"); return 4;
            }
            emit({lits[0], lits[1]}); cap2++;
        }
    }
    fprintf(stderr, "FIXED coordinate %d value %d points %zu free_target %d originals %d\n",
            coord, value, fixed.size(), free_target, originals);
    fprintf(stderr, "CAP lines %lld ternary %lld cross_binary %lld\n", lines, cap3, cap2);
    vector<int> wires(PN, FALSE_W);
    for (int v = 1; v <= originals; v++) wires[v - 1] = v;
    for (int p = 1; p < PN; p <<= 1)
        for (int k = p; k >= 1; k >>= 1)
            for (int j = k % p; j + k < PN; j += 2 * k)
                for (int i = 0; i < k; i++)
                    if ((i + j) / (2 * p) == (i + j + k) / (2 * p))
                        comparator(wires[i + j], wires[i + j + k], wires[i + j], wires[i + j + k]);
    int need = wires[free_target - 1];
    if (need == FALSE_W) { fprintf(stderr, "cardinality impossible\n"); return 5; }
    if (need != TRUE_W) emit({need});
    if (balance) {
        // Scaling only the fixed coordinate swaps the two free layers and fixes
        // every point in the fixed layer.  Hence some affine-equivalent solution
        // has at least ceil(free_target/2) points in the first free layer.
        const int LPN = 4096, first_free_value = (value + 1) % 3;
        vector<int> layer_wires(LPN, FALSE_W);
        int layer_points = 0;
        for (int x = 0; x < N; x++) if (coordinate(x, coord) == first_free_value)
            layer_wires[layer_points++] = variable[x];
        if (layer_points != 2187) { fprintf(stderr, "bad free layer size\n"); return 5; }
        for (int p = 1; p < LPN; p <<= 1)
            for (int k = p; k >= 1; k >>= 1)
                for (int j = k % p; j + k < LPN; j += 2 * k)
                    for (int i = 0; i < k; i++)
                        if ((i + j) / (2 * p) == (i + j + k) / (2 * p))
                            comparator(layer_wires[i + j], layer_wires[i + j + k],
                                       layer_wires[i + j], layer_wires[i + j + k]);
        int minimum_first = (free_target + 1) / 2;
        int layer_need = layer_wires[minimum_first - 1];
        if (layer_need == FALSE_W) { fprintf(stderr, "balanced cardinality impossible\n"); return 5; }
        if (layer_need != TRUE_W) emit({layer_need});
        fprintf(stderr, "BALANCED first_free_value %d minimum %d\n", first_free_value, minimum_first);
    }
    if (argc >= 8 && string(argv[7]) != "-") {
        set<int> witness = read_points(argv[7]);
        check_cap(witness);
        for (int x : fixed) if (!witness.count(x)) {
            fprintf(stderr, "witness lacks fixed point %d\n", x); return 6;
        }
        if ((int)witness.size() < target) {
            fprintf(stderr, "witness too small: %zu < %d\n", witness.size(), target); return 6;
        }
        for (int v = 1; v <= originals; v++) {
            int x = point_of_var[v];
            emit({witness.count(x) ? v : -v});
        }
        fprintf(stderr, "WITNESS points %zu units %d\n", witness.size(), originals);
    }
    long long final_clauses = clauses;
    int maxvar = nextvar - 1;
    fseek(out, 0, SEEK_SET);
    fprintf(out, "p cnf %010d %011lld\n", maxvar, final_clauses);
    fclose(out);
    fprintf(stderr, "DONE vars %d clauses %lld comparators %lld target %d\n",
            maxvar, final_clauses, comparators, target);
    return 0;
}
