# OpenSMA ESBMC Verification — Retraction Notes

Detailed analysis for each retracted finding. The summary table lives in
`REPORT.md § Retracted findings`; the reasoning for *why* each initial claim
was wrong is kept here so it doesn't clutter the main report.

---

## F-14 — `Pca9555::i2c_write` bare `return` on Input command

**File**: `src/nv/emulation/pca9555.cpp:142–175`

```cpp
CommandRegister = cmd_byte / 2;           // fixed for all iterations
for (uint8_t i = start_index; i < data_length; i++) {
    switch (CommandRegister) {
        case Input:
            return;    // bare return — but Input case has no body
        case Output: ...
    }
}
```

**Initially filed** based on an inline-model harness (not compiling the production source) that asserted `completed == expected` iterations and found a CEX. That analysis was incorrect.

**Why it was wrong**: `CommandRegister` is computed *once* before the loop as `cmd_byte / 2` and does not change across iterations. For any Input-register write (`cmd_byte ∈ {0, 1}`), `CommandRegister == Input` on every iteration. The `case Input` body is empty — there are no state mutations in that case. Therefore the bare `return` and a `break` produce **identical observable state**: `_gpio_output`, `_gpio_direction`, and `_gpio_inversion` are all unchanged either way. The commented-out `nv::warn("trying to write to PCA9555 input pin")` confirms the author treated this as an intentional caller-error path.

**What the upgraded harness confirmed** (`pca9555_f14_neg`, **VERIFICATION SUCCESSFUL — 405 VCC**):

The harness was rewritten to call the **actual production `Pca9555::i2c_write()`** from `pca9555.cpp` compiled by ESBMC (same rigor as F-1). It snapshots `output_before`, calls `dev.i2c_write(buf, 3)` with `buf[0]=0x00` (Input register) and nondet data bytes, then asserts `output_after == output_before`. ESBMC explored 405 verification conditions and found no violation. The production code path through `pca9555.cpp:142` was confirmed reachable and safe.

**Classification**: code-quality / documentation issue. The bare `return` is not wrong — it is observably equivalent to `break` for Input registers — but `break` would communicate intent more clearly, and the commented-out warn() line suggests the intent was never documented.

**Recommendation** (style only): replace `return` with `break` and add a comment explaining that Input registers are hardware-read-only and writes are silently ignored.

---

## Former F-6 — `buffer_to_uint32` cast-after-shift

**File**: `src/nv/telemetry/utils.cpp:31`

```cpp
| (static_cast<uint32_t>(buffer[3] << Byte3));  // cast after shift — initially flagged
```

The cast is applied after the shift rather than before. Initially filed as "signed-shift UB for `buffer[3] >= 0x80` under C++20." **Retracted**: C++20 P0907R4/P1236R1 makes signed left-shift fully defined for all inputs — the result is the unique value congruent to `E1 × 2^E2` modulo `2^N` — removing both the negative-E1 and result-overflow UB clauses that existed in C++17. ESBMC's bitvector model was already correct; `--overflow-check` rightly generates no VCC for this expression under `--std c++20`.

This is the same standard-conformance question as the retracted F-3 (`buf_to_u32`).

The code pattern is still a **style/portability issue**: bytes 0–2 cast before shifting (`static_cast<uint32_t>(buffer[N]) << Byte`) while byte 3 casts after. The recommended fix (move the cast before the shift) makes the intent uniform and correct even under C++17:
```cpp
| (static_cast<uint32_t>(buffer[3]) << Byte3)
```

**ESBMC issue filed as a consequence**: the misanalysis led to filing [esbmc#4240](https://github.com/esbmc/esbmc/issues/4240). That framing was wrong (no UB to miss), but #4240 uncovered a real ESBMC defect in the opposite direction: `--overflow-check` and `--ub-shift-check` were generating false-positive VCCs for signed shl under `--std c++20`. Fixed by [#4241](https://github.com/esbmc/esbmc/pull/4241). Repro retained at `esbmc_bug_repros/signed_shift_result_overflow.cpp`.

**What ESBMC verified** (`telemetry`, 80 VCC Phase 1 with `--ub-shift-check`, k=11 Phase 2): both the buggy and fixed forms satisfy the little-endian decoding contract across all 4-byte inputs.

---

## F-3 — `buf_to_u32` signed shift overflow

ESBMC's `--overflow-check` flagged `buf[start_idx] << ByteShift3` (i.e. `int(byte) << 24`) as an arithmetic-overflow violation when `byte >= 0x80`. Investigated:

- Production builds with `-std=c++23`. Under C++20+ ([expr.shift]/2), signed left-shift `E1 << E2` is well-defined: the value is the unique result congruent to `E1 × 2^E2` modulo `2^N` where `N` is the width of the result type. For `int(128) << 24`, that's `INT_MIN`; the surrounding `static_cast<uint32_t>(...)` then recovers the correct `0x80000000` bit pattern. **No UB.**
- Empirically validated: ESBMC's BMC proves `prod_form(b0, b1, b2, b3) == fixed_form(b0, b1, b2, b3)` for all four input bytes (0 VCCs after simplification — equivalence is structural). See `verification/ctest/f3/`.

Not a defect in OpenSMA. The standard-conformance gap that surfaced this — the default `--overflow-check` applying pre-C++20 UB rules irrespective of `--std` — was filed as [esbmc/esbmc#4201](https://github.com/esbmc/esbmc/issues/4201). The fix ([#4211](https://github.com/esbmc/esbmc/pull/4211)) uses a type-driven non-negativity predicate on `E1` (covers `uint8_t`/`uint16_t`-promoted operands — the OpenSMA case — without symbolic reasoning), with `--std c++20+` discrimination so legacy spellings stay strict. The equivalence ctest at `verification/ctest/f3/` is retained as a regression sentinel.

---

## F-2 — `align_to()` overflow

Initially claimed `align_to` had a real overflow bug. **It does not.**
The retraction is empirically validated via ESBMC's
`--branch-coverage --generate-ctest-testcase` on an equivalence harness
(`verification/ctest/f2/align_equiv.cpp`) that asserts
`align_buggy(v, A) == align_fixed(v, A)` for all inputs. ESBMC's BMC
explored the full input space without finding a counterexample to the
equivalence, and all 5 generated runtime test cases pass at execution time
(`100% tests passed, 0 tests failed`).

For unsigned arithmetic, `(value + alignment) - 1` and
`value + (alignment - 1)` are identically equal modulo 2³² because
addition and subtraction are modular and the operations cancel. Concretely,
for `value = alignment = 0x80000000`:

- `(0x80000000 + 0x80000000) - 1 = 0 - 1 = 0xffffffff (mod 2³²)`
- `0x80000000 + 0x7fffffff = 0xffffffff`

Both then go through `& ~(alignment - 1) = & 0x80000000` → `0x80000000`,
which is the correct aligned result. ESBMC's `--unsigned-overflow-check`
flagged the intermediate wrap on `value + alignment`, but unsigned wrap is
**defined behaviour** in C++ — the flag catches *unintended* wraps for
review, not bugs.

The `make utils_neg` harness still produces a counterexample (the wrap is
real, just benign); the harness is retained as a demonstrator of ESBMC's
strict unsigned-overflow flag, not as a regression sentinel for a defect.
The "fix" applied in `utils_harness.cpp` (parenthesising as
`value + (alignment - 1)`) is a readability/intent improvement that makes
the arithmetic match the guard's expression — adoption is a style choice,
not a correctness one.

**Lesson**: when ESBMC reports an `--unsigned-overflow-check` violation,
verify whether the wrap matters for the function's *output*. A wrap that
is reverted by a subsequent inverse operation is an artefact, not a bug.
