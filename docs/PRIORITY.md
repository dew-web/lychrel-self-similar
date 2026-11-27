# Discovery dates and public records

This page lists when each result of this repository was first made public, and the external records that show it.
Git commit dates are set by the author and are not evidence on their own; the dates below come from Zenodo, from GitHub's own server-side records, and from the Internet Archive.
(日本語の要約は末尾にある。)

## Archived versions (Zenodo)

| Version | DOI | Published (Zenodo record created, UTC) | Contents |
|---|---|---|---|
| all versions (concept DOI) | [10.5281/zenodo.23289048](https://doi.org/10.5281/zenodo.23289048) | – | always resolves to the latest version |
| first version | [10.5281/zenodo.23289049](https://doi.org/10.5281/zenodo.23289049) | 2026-10-10 22:49:03 (2026-10-11 07:49 JST) | the repository as of commit `af89ef4` |
| 2026.11.27 | see the concept DOI | 2026-11-27 | the repository as of the release `v2026.11.27` |

Each Zenodo version stores a zip file of the whole repository at that time, so its contents can be checked against the dates.

## GitHub's server-side push log

GitHub records the time of every push on its server; for a public repository the log is shown at https://github.com/dew-web/lychrel-self-similar/activity (and by the API `GET /repos/dew-web/lychrel-self-similar/activity`).

| Time (UTC) | Event | Commits |
|---|---|---|
| 2026-10-10 22:33:57 | branch `main` created | `af89ef4` (base-9 family, search programs, base-10 results) |
| 2026-10-10 22:36:53 | push | `af89ef4..c2a11d0` (LICENSE holder) |
| 2026-10-10 22:38:04 | push | `c2a11d0..066eecf` (CITATION.cff author) |
| 2026-11-21 07:45:37 | push | `066eecf..bc7fd7f` (the November results) |

A commit hash fixes the contents of the whole tree, so each row ties those contents to that time.

## Results and their first public record

| Result | First public record (UTC) | Evidence | Related work |
|---|---|---|---|
| Base-9 family N(m) with a center string, proof for all m ≥ 2 (`base9/`) | 2026-10-10 22:49 | Zenodo 10.5281/zenodo.23289049; GitHub release `Pre-release`, published 2026-10-10 22:48:59 | taco-jpg proved a different base-9 family (no center string, same overflow pattern 1000110001) on 2026-10-28 |
| Exact automaton search for two-run cycles (`src/outauto.c`, `src/coreauto2.c`, `docs/METHOD.md`) | 2026-10-10 22:49 | same | later reimplemented by Hughes for base 6 |
| Base 10: no two-run cycle of period 2–15 (even numbers of overflows) | 2026-10-10 22:49 | same | taco-jpg extended the end-string search to period 32 (2026-10-25 to 27) |
| Base 10: no two-run cycle of period 2–23; period 12 end strings (3 overflows) have no center | 2026-11-21 07:45 | GitHub push `066eecf..bc7fd7f` to `main` at 2026-11-21T07:45:37Z (GitHub's activity log); Zenodo version 2026.11.27 | the period-12 end strings were found first by taco-jpg |
| Families in bases 77, 235, 306, 390, 467 (`families/`, `scripts/symbolic_proof.py`) | 2026-11-21 07:45 | same | none found |
| Families in bases 50, 98, 146 | 2026-11-21 07:45 | same | proved earlier by mizopl, Zenodo 10.5281/zenodo.23468033 (2026-10-19) |
| Explicit base-20 cycle of period 8 (Seal's family) | 2026-11-21 07:45 | same | given earlier by taco-jpg (late October 2026); ours is an independent rediscovery |
| λ by overflow history; the b+2 jump in bases 2^b; the dip in bases 2^b+1 (`explore/relax*.c`) | 2026-11-21 07:45 | same | none found |
| Two-ended window model; growth rate (B+1)/(3B−2) for one pair; base dependence through B−1 and B−2 (`explore/window*.c`) | 2026-11-21 07:45 | same | the value 0.41393 itself follows from Walker's computation (1990) |

The palindrome criterion used in `p196/lemma_check.c` (x + R(x) is a palindrome only if all column sums are at most B−1, or all are 0 or B+1 with B+1 at the ends) is Theorem 3.7 of Ando (February 2026), and is not claimed here.

## Related work

- Ando, *Carry asymmetry, palindrome characterization, and conditional non-convergence in the reverse-and-add process*, February 2026, https://github.com/nkc-daiki/196-carry-asymmetry
- mizopl, preprint on Lychrel numbers in bases 50, 98 and 146, Zenodo, 2026-10-19, https://doi.org/10.5281/zenodo.23468033
- taco-jpg, *p196-algorithm* (Lean 4), https://github.com/taco-jpg/p196-algorithm
- T. B. Hughes, *Two infinite families of base-6 Lychrel numbers*, Zenodo, https://doi.org/10.5281/zenodo.24490763

## 日本語の要約

- 9進の族 N(m)（中心つき）、探索手法、10進の周期15以下の否定結果は、2026年10月11日（日本時間）に Zenodo の DOI 10.5281/zenodo.23289049 で公開した。
- 77・235・306・390・467進の族、10進の周期23以下の否定結果、λ の履歴依存、両端の窓モデルは、2026年11月21日に GitHub に公開し、2026年11月27日に Zenodo の新しい版として保存した。
- 50・98・146進は mizopl（2026年10月19日）、20進の明示的なサイクルと10進の周期12の両端の文字列は taco-jpg が先に示している。回文になる条件は Ando（2026年2月）の Theorem 3.7 と同じ。
