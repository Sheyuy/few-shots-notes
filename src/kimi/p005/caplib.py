"""Utilities for caps in F_3^8 using big-endian base-3 point ids.

id(p) = sum(p[i] * 3**(7-i) for i in range(8)).  Every archived .txt point
file in data/kimi/p005 uses this convention, one integer per line.
"""
from pathlib import Path

D = 8
N = 3**D


def load(path):
    return [int(s) for s in Path(path).read_text().splitlines()
            if s.strip() and not s.lstrip().startswith('#')]


def save(path, points, header=None):
    points = list(points)
    text = ([] if header is None else ['# ' + header]) + [str(x) for x in points]
    Path(path).write_text('\n'.join(text) + '\n')


def decode(x):
    return [(x // 3**(D-1-i)) % 3 for i in range(D)]


def encode(p):
    assert len(p) == D
    return sum(int(v) * 3**(D-1-i) for i, v in enumerate(p))


def third(a, b):
    """The third point on the line through distinct a and b: -(a+b)."""
    out, place = 0, 1
    for _ in range(D):
        out += (-(a % 3 + b % 3) % 3) * place
        a //= 3
        b //= 3
        place *= 3
    return out


def check_points(points):
    return (all(0 <= x < N for x in points) and
            len(points) == len(set(points)))


def conflict_lines(points):
    """Return all forbidden lines whose three points are selected."""
    points = list(points)
    selected = set(points)
    lines = set()
    for i, a in enumerate(points):
        for b in points[i+1:]:
            c = third(a, b)
            if c in selected:
                lines.add(tuple(sorted((a, b, c))))
    return sorted(lines)


def valid_cap(points):
    return check_points(points) and not conflict_lines(points)


def outside_conflicts(seed):
    """For each outside x, list edges (a,b) in seed with third(a,b)=x."""
    seed = list(seed)
    member = bytearray(N)
    for x in seed:
        member[x] = 1
    edges = [[] for _ in range(N)]
    for i, a in enumerate(seed):
        for b in seed[i+1:]:
            c = third(a, b)
            if not member[c]:
                edges[c].append((a, b))
    return edges


def directly_addable(seed):
    member = set(seed)
    return [x for x in range(N) if x not in member and
            all(third(x, a) not in member for a in seed)]
