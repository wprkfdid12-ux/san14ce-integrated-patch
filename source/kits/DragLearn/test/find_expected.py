#!/usr/bin/env python3
"""Independent finder (Python) for the host test: prints the 12 RVAs host_test expects.
usage: find_expected.py image.bin"""
import struct, sys
def parse(s): return [None if t == '??' else int(t, 16) for t in s.split()]
def find(d, pat, lo, hi):
    p = parse(pat); r = []
    for i in range(lo, hi - len(p)):
        if d[i] == p[0] and all(b is None or d[i + k] == b for k, b in enumerate(p)): r.append(i)
    return r
def rel(d, at): return at + 4 + struct.unpack_from('<i', d, at)[0]
d = open(sys.argv[1], 'rb').read()
def uniq(pat):
    p = parse(pat); fixed = bytes(p[:p.index(None)]) if None in p else bytes(p)
    r = []; k = d.find(fixed)
    while k != -1:
        if all(b is None or d[k + j] == b for j, b in enumerate(p)): r.append(k)
        k = d.find(fixed, k + 1)
    assert len(r) == 1, (pat, len(r)); return r[0]
S = uniq('BF 01 00 00 00 89 7D B8 48 89 7D BC B9 BC 05 00 00 E8') + 8
X = uniq('4D 8B 85 B8 04 00 00 4D 8B 88 A0 01 00 00 48 8B 54 24 38')
Y = uniq('48 8B 83 90 04 00 00 48 89 44 24 40 48 8B 83 78 04 00 00 48 89 44 24 48 48 89 BC 24 80 08 00 00 E8') + 32
learn = rel(d, Y + 1)
def inl(pat):
    r = find(d, pat, learn, learn + 0x600); assert len(r) == 1, pat; return r[0]
valid = rel(d, inl('48 8B F9 48 8B 09 E8 ?? ?? ?? ??') + 7)
db = rel(d, inl('0F B6 48 52 BD C8 00 00 00 48 8B 05 ?? ?? ?? ??') + 12)
slot = rel(d, inl('8B D5 48 8B 0F E8 ?? ?? ?? ?? 41 3B C7') + 6)
a = inl('48 8B 4F 18 E8 ?? ?? ?? ?? 48 8B D8 0F B6 48 14 2B 0D ?? ?? ?? ??'); orders = rel(d, a + 5); cost = rel(d, a + 18)
a = inl('48 8B 0D ?? ?? ?? ?? 48 85 C9 74 ?? 3B 05 ?? ?? ?? ??'); head = rel(d, a + 3); npool = rel(d, a + 14)
c = find(d, '48 8B 05 ?? ?? ?? ?? 48 83 3C C8 00', X, X + 0x40); assert len(c) == 1; cnt = rel(d, c[0] + 3)
P = uniq('BE 01 00 00 00 89 74 24 58 48 89 74 24 5C B9 2E 03 00 00 E8') + 9
Q = uniq('48 8D 4C 24 40 E8 ?? ?? ?? ?? 41 BE 00 40 01 00 48 8B 54 24 28 48 85 D2 74 04 8B 02 EB 03 49 8B C6 8B C8') + 10
def ins(pat):
    r = find(d, pat, S - 8 - 0x300, S - 8); assert len(r) == 1, pat; return r[0]
tid = rel(d, ins('49 8B CF E8 ?? ?? ?? ?? 8B F0 4D 8B B5 88 04 00 00 33 FF') + 4)
e2 = ins('48 8D B0 28 74 07 00 48 05 D0 74 07 00 48 3B F0 74 ?? 4C 8B 36 49 8B CE E8 ?? ?? ?? ?? 85 C0 74 ?? 49 8B CE E8 ?? ?? ?? ?? 8B F8 83 F8 14 77 ?? 41 8B 4F 6C 33 D2 0F A3 C1 0F 92 C2 85 D2 74 ?? 49 8B 8D 88 04 00 00 41 80 7E 7E 02 75 ?? E8 ?? ?? ?? ?? 3B C7 0F 84 ?? ?? ?? ?? EB ?? 8B D7 E8 ?? ?? ?? ?? 85 C0')
assert rel(d, e2 + 25) == valid
fid = rel(d, e2 + 37); pspec = rel(d, e2 + 79); pform = rel(d, e2 + 96)
gidx = rel(d, inl('48 8B 4F 08 E8 ?? ?? ?? ?? 4C 8B 47 18 8D 50 FF 83 FA 1D 77 ?? 48 63 C8 41 0F B7 8C 48 20 01 00 00') + 5)
pd = e2 + 0x66
assert find(d, '75 ?? 48 83 C6 08 48 8B 05 ?? ?? ?? ?? 48 05 D0 74 07 00 48 3B F0 75 ?? 83 3D ?? ?? ?? ?? 00 74', pd, pd + 40) == [pd]
Dsite = pd + 0x18; enable = pd + 2 + struct.unpack_from('<b', d, pd + 1)[0]; lock = Dsite + 7 + struct.unpack_from('<i', d, Dsite + 2)[0]
print(' '.join('%X' % v for v in (S, X, Y, learn, valid, slot, orders, db, cost, head, cnt, npool, P, Q, tid, fid, pspec, pform, gidx, Dsite, enable, lock)))
