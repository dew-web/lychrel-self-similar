# Self-similar reverse-and-add cycles, and a Lychrel family in base 9

Code and data for an exploration of the reverse-and-add ("196") problem:
repeat `x -> x + reverse(x)` and stop when `x` is a palindrome.
(日本語の説明は [README.ja.md](README.ja.md))

## Main results

**1. A Lychrel family in base 9.** For m >= 2 define, in base-9 digits,

```
N(m) = 1012563461128208006630705832188788502108582878207 [8 x m] 10581 [0 x m] 782087205811204808870238508037588811821254465208
```

Ten reverse-and-add steps map N(m) exactly to N(m+2), and no intermediate term is a palindrome.
Hence every N(m) is a base-9 Lychrel number. The smallest member N(2) has 106 base-9 digits
(decimal 15934552693951230432673150658604939453454726580180659172717647944825012746040357138556479060985866320).
Statement, one-period table and proof: [base9/RESULT.md](base9/RESULT.md).

As of October 2026 we found existence results only for powers of 2 and bases 11, 17, 20 and 26
(Wikipedia "Lychrel number"; K. S. Brown, MathPages; D. Seal, rec.puzzles 1996; OEIS A060382).
We found no mention of base 9. Confirmation by experts is pending.

**2. An exact search method for two-run self-similar cycles.** In the Seal–Brown type of cycle, the end strings and
a center string are separated by mirrored runs of 0 and B-1. Such runs only pass one carry bit, so the problem becomes finite:

- `outauto` decides, for a given period and overflow pattern, whether end strings of *any* length exist.
- `coreauto2` decides whether a center string of *any* length exists for the resulting carry signal.
- Both rebuild explicit numbers and verify them by direct simulation.

The search rediscovers Seal's families in bases 2, 4, 11 (including its 42–43-digit center) and 26.
See [docs/METHOD.md](docs/METHOD.md).

**3. Base 10.**
- There is no two-run self-similar cycle with period <= 15, for any lengths of the strings.
- Periods 16–20 are complete except for two overflow patterns that exceeded the state limit.
- The only near miss (period 11: end strings of 29 digits each that close up exactly) admits no compatible center of any length.

Tables: [results/SUMMARY.md](results/SUMMARY.md).

**4. The 196 trajectory** ([p196/](p196/)). An AVX2 reverse-and-add reaches 10^6 digits in about 4 minutes on one core,
reproducing Walker's pass count 2,415,836. The folder also has carry statistics and a heuristic decay-rate analysis.

## Quick start

```sh
make                 # build bin/*
make verify-base9    # quick check (m = 2..60) and the symbolic proof for all m >= 2
make validate        # rediscover bases 2, 4, 11, 26 and find the base-9 family (a few minutes)
```

Requirements: gcc and Python 3 (standard library only). `p196` needs an x86-64 CPU with AVX2.

## Layout

```
base9/      the base-9 result: RESULT.md, verify_quick.py, symbolic_proof.py, end-string windows
src/        outauto.c (end strings), coreauto2.c (center, outside-in), coreauto.c (center, inside-out),
            ssgraph.c (bounded graph search), verify_indep.py (independent long simulation)
scripts/    run_validation.sh, classify_base10.sh, summarize_logs.py
results/    SUMMARY.md, SUMMARY_tables.md, logs/ (raw outputs of all runs used in the summary)
p196/       196 trajectory code and results
docs/       METHOD.md
```

## Status and caveats

- The base-9 proof is computer-assisted. It is an exact finite computation, symbolic in m; please check `base9/symbolic_proof.py`.
- The base-10 classification covers only the two-run type and the stated periods.
  The "central uniform run" type was explored only with an earlier version, and those results are not included.
- An earlier version of `outauto` had a 64-bit state-key overflow. All results here come from the fixed version (256-bit keys).
- The novelty of the base-9 result has not yet been confirmed by experts.

## Acknowledgments

Developed with the assistance of an AI model (Claude, Anthropic).

## License and citation

MIT (see [LICENSE](LICENSE)). Citation metadata: [CITATION.cff](CITATION.cff).
