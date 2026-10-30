import sys
import time

sys.path.insert(0, '/workspace/few-shots-work/p005')
import layers

TIME_LIMIT = 300


def build_adj(C):
    L = sorted(C)
    n = len(L)
    idx = {x: i for i, x in enumerate(L)}
    out = []
    for i, a in enumerate(L):
        m = 0
        for j, b in enumerate(L):
            if i != j and layers.third7(a, b) not in C:
                m |= 1 << j
        out.append(m)
    return L, out


def greedy(L, adj):
    order = sorted(range(len(L)), key=lambda i: -bin(adj[i]).count('1'))
    clique = []
    cand = (1 << len(L)) - 1
    for v in order:
        if cand >> v & 1:
            clique.append(v)
            cand &= adj[v]
    return clique


def solve(C, init=None, limit=TIME_LIMIT):
    L, adj = build_adj(C)
    n = len(L)
    if init:
        pos = {x: i for i, x in enumerate(L)}
        b = [pos[x] for x in init]
        bs = set(b)
        assert all(bv in bs or True for bv in b)
        best = b
    else:
        best = greedy(L, adj)
    started = time.time()
    nodes = 0

    def expand(R, P):
        nonlocal best, nodes
        nodes += 1
        if time.time() - started > limit:
            return True
        if not P:
            if len(R) > len(best):
                best = list(R)
            return False
        if len(R) + bin(P).count('1') <= len(best):
            return False
        while P:
            v = (P & -P).bit_length() - 1
            P &= P - 1
            if len(R) + bin(P).count('1') + 1 <= len(best):
                continue
            if expand(R + [v], P & adj[v]):
                return True
        return False

    expand([], (1 << n) - 1)
    return {L[i] for i in best}, n, len(best), nodes, time.time() - started


if __name__ == '__main__':
    L = layers.load()
    for i in (0, 1, 2):
        C = layers.candidates(L, i)
        sol, n, k, nodes, dt = solve(C, init=L[i])
        print('layer', i, '|C|', n, 'current', len(L[i]), 'best clique', k, 'nodes', nodes, 'time', round(dt, 1), flush=True)
    sys.stdout.flush()