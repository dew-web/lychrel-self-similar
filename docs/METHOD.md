# Method: two-run self-similar cycles as finite automata

## Setting

Write B for the base and ω = B-1. A *two-run* (Seal–Brown type) number has the form

```
[top string] [run of ω or 0] [center string] [run of 0 or ω] [bottom string]
```

The center string is centered, so the two runs are mirror images of each other (one run of 0, one of ω).
A *self-similar cycle of period P* is a family x(n) for which P reverse-and-add steps map x(n) to x(n+δ).
Such a family never reaches a palindrome, so every member is a Lychrel number.

## Decomposition

Every column sum inside the runs equals ω, so a run passes the incoming carry through unchanged and stays uniform.
One step therefore interacts with the three finite windows only through three bits:

- `cin`: the carry leaving the bottom window, which travels through the low run into the center;
- `cout`: the carry leaving the center, which travels through the high run into the top window;
- `ov`: the overflow at the top (the number gains a digit; the center shifts by half a position).

Both runs keep the 0/ω mirror structure exactly when `cout = 1 - cin`, i.e. when the center inverts the carry at every step.
So a cycle is the same thing as:

- an *outer* periodic orbit (top and bottom windows) whose top input is 1 - (its own bottom output), together with
- a *center* periodic orbit that inverts the same carry signal under the same overflow pattern.

## `outauto`: end strings of unbounded length

- **Construction.** Builds the end strings from the outermost digit pair inward, for a fixed period P and overflow pattern.
- **State.** For each phase: the bottom carry, the required carry out of the top digit, and a pending digit (on overflow the top window shifts by one).
- **Acceptance.** The state hands the carries over to the runs consistently.
- **Completeness.** The reachable state set is finite and explored completely (breadth-first search with a hash table). Hence "no accepting state" means no end strings of any length exist for that pattern.
- **Modes.** `Aa` lists every distinct carry signal and writes the windows to `outer_B*_P*_ov*_cin*.txt`.
- **Key size.** States are packed into 256-bit keys. An earlier 64-bit version overflowed for long patterns and is not included.

## `coreauto2`: centers of unbounded length

- **Construction.** Builds the center from the runs inward, starting from the "inside the runs" state.
- **Half-step shift.** On overflow the center moves by half a position. This is handled by giving each phase its own layer offset.
  Over one period the offsets grow by δ = (number of overflows)/2, so the state keeps the last δ layers of phase-0 digit pairs to check periodicity.
- **Closing.** From every reachable state, a closing sequence of δ+1 layers is tried. In it the phases reach their centers one after another.
- **Output.** If closing succeeds, the center is reconstructed, checked by an independent window simulation, and combined with the end windows into an explicit number. That number is then verified by direct reverse-and-add simulation.
- **Completeness.** If the reachable set is exhausted without a successful closing, no center of any length exists for that signal.
- **Why outside-in.** Starting from the runs is far more efficient than building from the center (`coreauto.c`). There, the periodicity check starts only after δ layers, and the state set explodes.

## Validation

- Two independent center automata agree. Example: base 10, period 9, a test signal gives a 3-digit center for one parity and "none of any length" for the other.
- The pipeline rediscovers known families (`scripts/run_validation.sh`):
  - base 2, period 4;
  - base 4, period 6;
  - base 26, period 4 (8-digit end strings, as in Seal 1996);
  - base 11, period 6 (7-digit end strings and a 42–43-digit center, as in Seal 1996).
- Every family found is verified by direct simulation of explicit numbers.

## Limits

- State limits: outauto 2^23 or 2^21 states per pattern; coreauto2 2^24.
- Only two-run cycles are covered. Cycles with more runs, or with a central uniform run, are not covered here.
