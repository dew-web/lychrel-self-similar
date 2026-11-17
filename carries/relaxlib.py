import numpy as np
def step(d, B):
    s = d + d[::-1]; n = len(s)
    g = s >= B; prop = s == B - 1
    idx = np.where(~prop, np.arange(n), -1); last = np.maximum.accumulate(idx)
    cout = np.where(last >= 0, g[np.maximum(last, 0)], False)
    cin = np.concatenate(([0], cout[:-1].astype(np.int64)))
    y = (s + cin) % B
    if cout[-1]: return np.concatenate((y, [1])), True
    return y, False
def lam_from(hist, pairs, seqs, js=np.arange(4, 17)):
    L = np.arange(len(hist))
    W = [(hist * np.maximum(0, L - j + 1)).sum() / (pairs - (j - 1) * seqs) for j in js]
    return float(np.exp(np.polyfit(js, np.log(W), 1)[0]))
