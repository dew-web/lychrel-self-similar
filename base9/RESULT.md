# A Lychrel family in base 9

Let F(x) = x + reverse(x), computed in base 9. For m >= 2 define (base-9 digits, most significant first)

```
N(m) = 1012563461128208006630705832188788502108582878207 [8 x m] 10581 [0 x m] 782087205811204808870238508037588811821254465208
```

**Theorem.** For every m >= 2, F^10(N(m)) = N(m+2), and none of F^t(N(m)) for 0 <= t <= 9 is a palindrome.

**Corollary.** For every m >= 2, the trajectory N(m), F(N(m)), F^2(N(m)), ... never reaches a palindrome.
So N(m), and every number on its trajectory, is a Lychrel number in base 9.
The smallest member, N(2), has 106 base-9 digits:

```
decimal: 15934552693951230432673150658604939453454726580180659172717647944825012746040357138556479060985866320
```

## One period

Run lengths are written in terms of m (output of `symbolic_proof.py`).

```
t=0 : 1012563461128208006630705832188788502108582878207 [8^(m+0)] 10581 [0^(m+0)] 782087205811204808870238508037588811821254465208
t=1 : 10038318082420407865038765253087883523304621782178 [0^(m-1)] 105610 [8^(m-1)] 7821881164823264788782634578414607841133008227310
t=2 : 11421608125535378616537418882887740756251741705167 [8^(m-2)] 0271621 [0^(m-2)] 86051703815255814777828002472472578343562100621311
t=3 : 22734308253170777245065840075876582722513672521246 [8^(m-2)] 1543442 [0^(m-2)] 73212518631532620667757884056144376707224281333722
t=4 : 45567627505451655600242000062764285446127464142484 [8^(m-2)] 4087004 [0^(m-2)] 47425147363155450346726000023208665515360571677544
t=5 : 10125634611281332338047400013563860201226484830506 [8^(m+0)] 10581 [0^(m+0)] 6850304845331018604553000047410432231821254465208
t=6 : 100383180824204555740532400050214651214582435236467 [0^(m-1)] 105610 [8^(m-1)] 67454254417541226542184000532503755551133008227310
t=7 : 114216081255361224145767400541461373361406887702346 [8^(m-2)] 0271621 [0^(m-2)] 743186788714053384054244004767552422053562100621311
t=8 : 227343082531721448412645801084021866721825886483704 [8^(m-2)] 1543442 [0^(m-2)] 486405688418226767228400010646203844227224281333722
t=9 : 4556762750545440077154028110888447454447418840884 [8^(m+0)] 4087004 [0^(m+0)] 4801488047355546348880120303418788355360571677544
t=10: N(m+2)
```

## Proof

**Run lemma.** Let x = A a^L C c^L E in base B, with a + c = B - 1 and L >= | |A| - |E| |.
Pair each position p with its mirror D-1-p, and split the positions into five zones:

- two run zones, where a digit of one run faces a digit of the other run;
- three fixed zones: the low end, the center, and the high end.

In the run zones every column sum is a + c = B - 1. So the incoming carry passes through unchanged, and the output digits there are all equal.
The fixed zones have lengths and contents that do not depend on L.
Hence F(x) = A' r^(L-d) C' r'^(L-d) E', where d = | |A| - |E| | and where A', C', E', r and r' do not depend on L.
(r and r' are the run digits after the step. They are 0 and B-1 in some order exactly when the carries entering the two run zones differ.)

**The ten steps.** `symbolic_proof.py` computes each step with every run length kept as a linear form a*m + b:

- It merges the boundaries of x and reverse(x), checking that their order is the same for all m >= 4.
- It computes the constant column sum of each interval.
- It propagates carries interval by interval. Inside an interval the carry is always 1 if the sum is >= 9, always 0 if the sum is <= 7, and passed through if the sum is 8.

Result: for all m >= 4, step t maps the t-th form exactly to the (t+1)-th form, and step 9 gives N(m+2).
For m = 2 and m = 3 the ten steps are computed directly.

**No palindromes.** In every form the 8-run and the 0-run occupy mirrored positions, so the digit pattern is not symmetric.
For m >= 4 this holds symbolically. For m = 2 and m = 3 it is checked directly.

Induction on the number of periods completes the proof. ∎
(For m = 0 and m = 1 the statement fails: the runs are too short, and the end and center strings interact.)

## How to check

```sh
python3 base9/verify_quick.py      # 20 lines: m = 2..60, one period each
python3 base9/symbolic_proof.py    # proof for all m >= 2 (symbolic in m)
python3 src/verify_indep.py        # long direct simulation (1500 steps, several run lengths), independent code
```

## How it was found, and variants

- The end strings (49–51 digits per end) come from `outauto` (base 9, period 10, overflow pattern 1000110001).
- The center comes from `coreauto2`. It must invert the carry signal 0111101111 at every step.
- The orientation-flipped family (carry signal 1000010000) and centers of the other parity (6 or 11 digits) also work.
  See `results/logs/scan3_core.log`; all four were verified by direct simulation.
