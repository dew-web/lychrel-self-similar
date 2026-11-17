#!/usr/bin/env python3
"""relax_gpu.py — relax2.c と同じ実験を GPU（PyTorch）で行う。
一様乱数の D 桁の数を大量に作って逆順加算をくり返し、桁が増えた直後の項（O の直後）について、
その前に桁が増えないステップ（N）が何回続いたか（n）ごとに、良い対の連なりの減衰率 λ(n) を測る。

usage: relax_gpu.py B [--digits 3000] [--samples 1000000] [--batch 50000] [--steps 80] [--skip 20] [--seed 1]
繰り上がりは「最後に素通しでなかった桁」を cummax で求めることで、桁方向のループなしに計算する。"""
import argparse, math, time, torch

ap = argparse.ArgumentParser()
ap.add_argument("B", type=int); ap.add_argument("--digits", type=int, default=3000)
ap.add_argument("--samples", type=int, default=1000000); ap.add_argument("--batch", type=int, default=50000)
ap.add_argument("--steps", type=int, default=80); ap.add_argument("--skip", type=int, default=20)
ap.add_argument("--seed", type=int, default=1); ap.add_argument("--cpu", action="store_true")
a = ap.parse_args()
dev = torch.device("cpu" if a.cpu or not torch.cuda.is_available() else "cuda")
B, D0, K, JMAX, NMAX = a.B, a.digits, a.steps, 24, 16
DM = D0 + K + 2
g = torch.Generator(device=dev); g.manual_seed(a.seed)
pos = torch.arange(DM, device=dev, dtype=torch.int32)
cnt = torch.zeros((NMAX + 1) * (JMAX + 1), dtype=torch.float64, device=dev)   # [n][min(run, JMAX)] の度数
tot = torch.zeros(NMAX + 1, JMAX + 1, dtype=torch.float64, device=dev)
nev = torch.zeros(NMAX + 1, dtype=torch.float64, device=dev)
t0 = time.time(); done = 0
while done < a.samples:
    N = min(a.batch, a.samples - done); done += N
    x = torch.randint(0, B, (N, DM), generator=g, device=dev, dtype=torch.int32)
    L = torch.full((N,), D0, device=dev, dtype=torch.int32)
    x[:, D0 - 1] = torch.randint(1, B, (N,), generator=g, device=dev, dtype=torch.int32)   # 先頭の数字は 0 でない
    x[:, D0:] = 0
    nrun = torch.full((N,), -1, device=dev, dtype=torch.int32)                  # 直前の O からの N の回数（最初の O までは -1）
    for k in range(1, K + 1):
        valid = pos[None, :] < L[:, None]
        ridx = (L[:, None] - 1 - pos[None, :]).clamp(min=0).long()
        s = torch.where(valid, x + torch.gather(x, 1, ridx), torch.zeros_like(x))
        # 繰り上がり: 列和が B-1 の桁は素通し。最後に素通しでなかった桁が B 以上なら 1
        nonprop = torch.where(s != B - 1, pos[None, :].expand(N, DM), torch.full_like(s, -1))
        last = torch.cummax(nonprop, dim=1).values
        cout = torch.where(last >= 0, (torch.gather(s, 1, last.clamp(min=0).long()) >= B).int(), torch.zeros_like(s))
        cin = torch.zeros_like(s); cin[:, 1:] = cout[:, :-1]
        y = s + cin; y = torch.where(y >= B, y - B, y)
        ov = torch.gather(cout, 1, (L[:, None] - 1).long()).squeeze(1).bool()   # 最上位からの繰り上がり = 桁が増える
        y = torch.where(valid, y, torch.zeros_like(y))
        y[torch.nonzero(ov).squeeze(1), L[ov].long()] = 1
        x = y; L = L + ov.int()
        if k > a.skip:
            sel = ov & (nrun >= 0)
            if sel.any():
                xs, Ls, cls = x[sel], L[sel], nrun[sel].clamp(max=NMAX).long()
                H = (Ls // 2)
                r = (Ls[:, None] - 1 - pos[None, :]).clamp(min=0).long()
                inhalf = pos[None, :] < H[:, None]
                good = (xs + torch.gather(xs, 1, r) <= B - 1) & inhalf
                lastbad = torch.cummax(torch.where(good, torch.full_like(xs, -1), pos[None, :].expand_as(xs)), dim=1).values
                run = (pos[None, :] - lastbad).clamp(max=JMAX)                   # 位置 i で終わる良い対の連なりの長さ
                run = torch.where(inhalf, run, torch.zeros_like(run)).long()
                cnt += torch.bincount((cls[:, None] * (JMAX + 1) + run).flatten(), minlength=(NMAX + 1) * (JMAX + 1)).double()
                j = torch.arange(JMAX + 1, device=dev)
                tot.index_add_(0, cls, (H[:, None] - j[None, :] + 1).clamp(min=0).double())
                nev.index_add_(0, cls, torch.ones_like(cls, dtype=torch.float64))
        nrun = torch.where(ov, torch.zeros_like(nrun), torch.where(nrun >= 0, nrun + 1, nrun))
cnt = cnt.view(NMAX + 1, JMAX + 1).cpu(); tot = tot.cpu(); nev = nev.cpu()
W = torch.flip(torch.cumsum(torch.flip(cnt, [1]), 1), [1])                     # W[n][j] = 連なりが j 以上の位置の数
def lam(Wn, Tn):
    xs, ys = [], []
    for j in range(4, 17):
        if Wn[j] <= 0: return float("nan")
        xs.append(j); ys.append(math.log(Wn[j] / Tn[j]))
    n = len(xs); sx, sy = sum(xs), sum(ys); sxx = sum(v * v for v in xs); sxy = sum(u * v for u, v in zip(xs, ys))
    return math.exp((n * sxy - sx * sy) / (n * sxx - sx * sx))
print(f"# B={B} D={D0} samples={a.samples} steps={a.skip + 1}..{K} device={dev} time={time.time() - t0:.1f}s")
print("# n events frac lambda W1")
allW, allT, alln = W.sum(0), tot.sum(0), nev.sum()
for n in range(NMAX + 1):
    if nev[n] > 0: print(f"{n} {int(nev[n])} {nev[n] / alln:.5f} {lam(W[n], tot[n]):.4f} {W[n][1] / tot[n][1]:.4f}")
print(f"all {int(alln)} 1.00000 {lam(allW, allT):.4f} {allW[1] / allT[1]:.4f}")
