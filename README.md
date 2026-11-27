# Self-similar reverse-and-add cycles, and a Lychrel family in base 9

[![DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.23289048.svg)](https://doi.org/10.5281/zenodo.23289048)

Archived at Zenodo: all versions [10.5281/zenodo.23289048](https://doi.org/10.5281/zenodo.23289048);
first version (11 October 2026) [10.5281/zenodo.23289049](https://doi.org/10.5281/zenodo.23289049);
November results (released 27 November 2026) [10.5281/zenodo.24581754](https://doi.org/10.5281/zenodo.24581754).
When and where each result was first made public: [docs/PRIORITY.md](docs/PRIORITY.md).

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
Concurrent work since then (mizopl: bases 50, 98, 146; taco-jpg: bases 13 and 20 and a different base-9 family; Hughes: base 6) is listed in [docs/PRIORITY.md](docs/PRIORITY.md).

**2. An exact search method for two-run self-similar cycles.** In the Seal–Brown type of cycle, the end strings and
a center string are separated by mirrored runs of 0 and B-1. Such runs only pass one carry bit, so the problem becomes finite:

- `outauto` decides, for a given period and overflow pattern, whether end strings of *any* length exist.
- `coreauto2` decides whether a center string of *any* length exists for the resulting carry signal.
- Both rebuild explicit numbers and verify them by direct simulation.

The search rediscovers Seal's families in bases 2, 4, 11 (including its 42–43-digit center), 17 and 26, and an explicit base-20 cycle of period 8 (found independently; given earlier by taco-jpg).
See [docs/METHOD.md](docs/METHOD.md).

**3. Base 10.**
- There is no two-run self-similar cycle with period <= 23, for any lengths of the strings; of the 349,715 overflow patterns of period 24, only `010101010101010101010101` is open (November 2026, `docs/NOTES-2026-11-16.md`).
- The October 2026 tables covered only overflow patterns with an even number of overflows. The period-12 pattern `000100010001` (three overflows) has end strings, found first by taco-jpg; its center does not exist.
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

## Added in November 2026
- `carries/`: exact carry-including transfer operator for the decay rate (`op.py`) and relaxation experiments (`relax_perstep.py`, `relax_buckets.py`).
- `scripts/halfsearch.py`: fast search restricted to doubled (ww) overflow patterns; results in `results/halfsearch_2026-10.md`.
- `docs/RUNBOOK.md`: how to run everything on another computer (Japanese). `make env` prints the environment, `make check` runs a smoke test.
- **More Lychrel families** (`families/`): two-run self-similar families in bases 50, 77, 98, 146, 235, 306, 390 and 467, each proved for all run lengths by `scripts/symbolic_proof.py` (the base-9 proof generalized to any base) and checked independently by `scripts/verify_family.py`. Bases 77, 235, 306, 390 and 467 appear to be new; Lychrel numbers in bases 50, 98 and 146 were proved earlier by mizopl (Zenodo, 19 October 2026). Not yet reviewed by an expert.
- **Base 10**: the two-run type has no self-similar cycle of period 2-23; of the 349,715 overflow patterns of period 24, only `010101010101010101010101` is still open. Details in `docs/NOTES-2026-11-16.md`.
- **New search programs**: `src/outauto2.c` (outer automaton with the rotation symmetry factored out), `src/outauto2p.c` (OpenMP), `src/coreauto2w.c` and `src/coreauto2wp.c` (center automaton with 256-bit keys). `make new`, `make parallel`.
- **Two-ended window model** (`explore/window*.c`, `docs/NOTES-2026-11-18-window.md`): following only the outer few digits at each end, with the carry from the middle replaced by a coin, reproduces the growth of 196 (0.41394 digits per step against 0.41393 measured).
- The draft paper on the base-9 family is kept private until it has been checked by an expert.
