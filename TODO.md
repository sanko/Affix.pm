# Affix.pm TODO

## Potential Optimizations (Call Path)

This section records optimization ideas for the normal Affix FFI call path,
resulting from the cross-platform benchmark investigation in November 2026.

Ranked roughly by expected payoff / effort.

---

### 1. Specialized monolithic XSUBs for common signatures
**Status:** Ideas only — not prototyped.
**Payoff:** High. **Effort:** High.

The current `GENERATE_TRIGGER_XSUB` is fully generic: a computed-goto dispatch
loop per argument, an indirect `affix->cif()` call, a `switch (affix->ret_opcode)`
on the return, and three separate `AFFIX_ALLOCA` calls — on *every* invocation.
That is the structural reason XS beats Affix by 1.4–2.7x on some platforms.

At `affix()`/`wrap()` time the exact signature is already known. Store a
`fast_call` function pointer in `Affix` and, for all-primitive-scalar
signatures, use a hand-generated C template that:

- reads `ST(0)..ST(n-1)` directly (reusing the `&SvNVX` / `&SvIVX` / `&SvPVX`
  direct-pointer tricks from the current dispatch cases),
- calls `affix->cif` once,
- writes the return straight into `TARG` with the correct type baked in,
- skips the dispatch table, the `ret_opcode` switch, and the out-param loop.

Instantiate templates via X-macros for the ~30 most common combinations
(arity <= 3, primitive types). Fall back to the generic path otherwise.
This captures the upside of `direct_affix` without its per-argument
marshaller-call problem.

---

### 2. Constant-16 return alignment
**Status:** REJECTED. Implemented on branch `bench/opt-235` (combined with #3
and #5) and validated on all six platforms in CI runs `44863288263` and
`44868560954`. Full suite passed (385 tests), but the combined change produced
a reproducible 10–11% drop in Linux arm64 plain `Affix` *and* `Affix wrap`
`strlen` throughput while XS, Inline::C and FFI::Platypus stayed flat. That is
the same signature that got Idea 4 reverted. The branch was dropped; `dev` is
unchanged. Individual contribution of #2 was not isolated.
**Payoff:** Low-Medium. **Effort:** Low.

Idea 4 (runtime bitmask alignment, commit `05b04a9`, later reverted in
`652cda5`) regressed Linux arm64 by 6–14% while every reference implementation
stayed flat or improved. The likely culprit: the bitmask still *loaded*
`ret_align` from memory at runtime.

`alloca` on all target compilers returns memory aligned to at least 16 bytes.
Allocate `ret_size + 16` and use `(addr + 15) & ~(UV)15` with a **compile-time
constant** — two ALU ops, no memory load — keeping the existing division path
only for types with `ret_align > 16`. This is strictly fewer instructions than
both the original division and Idea 4.

---

### 3. Remove the redundant rebuild check
**Status:** REJECTED as part of `bench/opt-235` (combined with #2 and #5).
Individual contribution not isolated; the combined change regressed Linux arm64
`strlen`, so none of the three were landed. The reasoning still appears sound
and this could be retried alone, but the gain is only one branch per call.
**Payoff:** Low. **Effort:** Trivial.

The XSUB prologue checks `if (UNLIKELY(!affix->infix)) rebuild_affix_data(...)`
twice. `rebuild_affix_data` always sets `affix->infix` (via
`infix_forward_create_manual`) and croaks on failure, so the second check is
unreachable. Removing it saves one branch per call.

---

### 4. Direct SV field writes for scalar returns
**Status:** REJECTED. Implemented on branch `bench/sv-return` (commit `cbb969d`)
using Perl's own `TARGi` / `TARGu` / `TARGn` fast-path macros. Full suite passed
(385 tests). CI run `44868609619` and a local Windows x64 A/B showed consistent
regressions on Windows x64 (−7% to −12% drift-corrected) and macOS `strlen`
(−27% to −40%). Likely cause: when `dXSTARG` yields a fresh mortal in void
context, the macros fall back to the slower `sv_set*_mg` variants instead of the
original `sv_set*`. The branch was dropped; `dev` is unchanged.
**Payoff:** Low-Medium. **Effort:** Moderate.

`sv_setiv(TARG, x)` / `sv_setnv(TARG, x)` are real function calls with
magic/readonly checks. For a freshly allocated `dXSTARG` the SV is known
clean; direct `SvIVX`/`SvNVX` writes plus flag manipulation could remove a PLT
call per invocation. Needs careful Perl-version guarding.

---

### 5. Coalesce the three `alloca` calls
**Status:** REJECTED as part of `bench/opt-235` (with #2 and #3). The coalesced
path had a correctness guard (`ret_align > 16` fallback) and passed all 385
tests, but the combined change caused the Linux arm64 `strlen` regression
described in #2. Individual contribution not isolated.
**Payoff:** Low. **Effort:** Moderate.

`args_buffer`, `c_args`, and `ret_buffer` are three separate stack
allocations. Replace with one allocation partitioned into aligned regions
(saving two stack-pointer adjustments per call) when all three fit under the
2 KiB threshold; keep the existing slow paths otherwise.

---

### 6. Bypass the trampoline for trivial signatures
**Status:** Ideas only — not prototyped.
**Payoff:** High. **Effort:** High, platform-specific.

For `double -> double` on SysV, the ABI is just "argument in XMM0, result in
XMM0". A tiny inline-asm stub (`movsd; call *target; movsd`) avoids the JIT
trampoline call entirely — this is what hand-written XS does implicitly. Only
viable for a fixed set of signatures x ABIs; must live behind strict `#if`.

---

### 7. Inline the marshallers in the JIT trampoline
**Status:** Ideas only — not prototyped. **Constraint:** any work here stays in
WSL scratch trees until explicitly requested; `infix/` in the Windows tree
remains untouched.
**Payoff:** High. **Effort:** High.

The direct path's weakness is a per-argument marshaller function call. If
Infix generated trampoline code that loaded directly from SV field offsets
instead, `direct_affix` would become genuinely faster than plain `affix`.

---

### 8. Whole-call JIT
**Status:** Ideas only — not prototyped.
**Payoff:** Maximum. **Effort:** Maximum.

Generate machine code for the entire XSUB body — SV reads, native call, SV
write — as one blob. No dispatch, no Perl XS frame. Essentially reimplementing
the normal path in machine code; could match or beat XS. Very high complexity
and portability cost.

---

### 9. Benchmark methodology notes (not code changes)

- CI runner variance is large: macOS arm64 XS swung +25% between two runs of
  identical code. For article claims, prefer within-run ratios over cross-run
  deltas.
- Idea 4's local A/B favored it while CI arm64 did not — a reminder that
  single-machine A/B can mislead on cross-platform work. Any future
  micro-optimization should be validated on all six platforms before landing.
- Auto-switching to `direct_affix` remains deferred: plain `affix` is now
  competitive everywhere (plain/direct ratio 0.85–1.18 across the six
  platforms), so auto-switching would be a regression on some of them. The
  thread-clone concern also still needs a closer look.

---

## Benchmark runs used for these decisions

| Run | Code | Purpose |
|---|---|---|
| `44849630164` | canonical baseline (`8c6cf0f` / `652cda5`) | Article dataset; tree identical to current `dev` C code |
| `44863288263` | `bench/opt-235` r1 (#2 + #3 + #5) | First six-platform validation of the coalesced-allocation change |
| `44868560954` | `bench/opt-235` r2 | Repeat to separate code effects from runner noise |
| `44868609619` | `bench/sv-return` (#4) | Six-platform validation of direct TARG writes |
| `44868609619` rerun | `bench/sv-return` (#4) | Repeat under a heavily loaded runner; Linux x64 Affix/XS `sin` reached 0.98 |
| `44904811037` | current `dev` (C code identical to baseline) | Control under current runner state: Linux x64 Affix/XS `sin` fell to 0.56 |

Conclusions:

- The macOS arm64 `sin` drop seen in all three later runs is environmental:
  it appears in `sv-return` too, which shares none of the 2/3/5 changes.
- The Linux arm64 `strlen` drop remains the best-supported code effect: it
  appears in both opt-235 runs in plain `Affix` and `Affix wrap` only, while
  every reference implementation stays flat and `sv-return` returns to
  baseline. The newly documented Affix-only run variance makes this less
  certain than the original rejection implied, but not enough to revisit it.
- Local single-machine A/B is useful for screening but not for landing: idea 4
  and ideas 2/3/5 both looked good locally and failed on arm64 in CI.
- Runner drift between runs can exceed 40%; always compare against XS and
  Inline::C *within the same run* before attributing a change to code.
- Even that correction is insufficient on its own. Run `44904811037` used C
  code identical to the canonical baseline, yet Linux x64 `sin` Affix
  throughput was 17.25M vs 21.50M and the Affix/XS ratio swung from 0.73 to
  0.56, while XS, Inline::C and FFI::Platypus stayed within 5% of baseline.
  Drift correction therefore reported a spurious -22% "regression" for
  unchanged code. Single-run Affix/XS ratios on identical code span 0.56-0.98;
  no article claim should rest on one of them.
