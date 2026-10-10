# 196 trajectory experiments

- `p196.c` — AVX2 reverse-and-add (carries via one 64-bit addition of (G|P)+G per 64 digits), with statistics and checkpoint restart.
  `./bin/p196 --selftest`, `./bin/p196 1000000` (about 4 min on one core). Reproduces Walker's 10^6 digits at pass 2,415,836; about 0.41393 digits per step.
- `lemma_check.c` — exhaustive check for x < 10^8 of the palindrome criterion. x + R(x) is a palindrome exactly when:
  - all column sums are <= 9 (no carry), or
  - all column sums are in {0, 11} and the outer sum is 11 (the number gains a digit).
- `hazard.c`, `run_hazard.sh`, `analyze.py` — per-step palindrome probability for random 15-digit seeds, compared with window statistics of the 196 trajectory.
  The decay per digit pair is about 0.64 right after a digit-gaining step.
- `base2_vs_base10.py` — the period-4 pattern of 22 = 10110 in base 2, and a compressibility comparison with the 196 trajectory.
- `results/` — summaries of the 10^6-digit run and the hazard experiment.
