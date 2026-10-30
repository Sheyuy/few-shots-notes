import sys

sys.path.insert(0, '/workspace/few-shots-work/p005')
import layers
import layers_exact


def expanded(L, target, rem_layer, r):
    i, j = target, rem_layer
    k = 3 - i - j
    C = layers.candidates(L, i)
    S_j = L[j] - {r}
    S_k = L[k]
    B = {layers.third7(r, b) for b in S_k}
    add = set()
    for x in B:
        if x in C:
            continue
        ok = True
        for b in S_j:
            if layers.third7(x, b) in S_k:
                ok = False
                break
        if ok:
            for b in S_k:
                if layers.third7(x, b) in S_j:
                    ok = False
                    break
        if ok:
            add.add(x)
    return C | add


def main():
    L = layers.load()
    base_total = sum(len(L[i]) for i in range(3))
    print('start', base_total, [len(L[i]) for i in range(3)], flush=True)
    results = []
    for target in (0, 1, 2):
        for rem_layer in ((target + 1) % 3, (target + 2) % 3):
            print('phase', target, rem_layer, 'points', len(L[rem_layer]), flush=True)
            for cnt, r in enumerate(sorted(L[rem_layer])):
                C = expanded(L, target, rem_layer, r)
                sol, n, k, nodes, dt = layers_exact.solve(C, init=L[target], limit=30)
                j = rem_layer
                kk = 3 - target - j
                total = len(sol) + (len(L[j]) - 1) + len(L[kk])
                if total > base_total:
                    print('IMPROVE', total, 'target', target, 'removed layer', j, 'point', r,
                          'target clique', len(sol), 'C', n, flush=True)
                    return
                results.append((total, target, j, r, len(sol), n))
                if cnt % 40 == 0:
                    print('  ..', cnt, 'best total so far', max(x[0] for x in results), flush=True)
    results.sort(reverse=True)
    print('best single swaps', results[:5], flush=True)
    print('no improvement over', base_total, flush=True)


if __name__ == '__main__':
    main()