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
**Status:** Implemented in scratch worktree `affix-opt-235` (combined with #3
and #5). Full suite passes (385 tests). Local Windows x64 A/B (4 trials each,
3 CPU-sec): sin +15–16%, abs ~flat, strlen +4–5% — but noisy, and the `sin`
result is partly driven by a low baseline outlier. **Cross-platform CI
validation still required before landing.** No push has been made.
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
**Status:** Implemented in scratch worktree `affix-opt-235` (combined with #2
and #5). Part of the A/B above; individual contribution not isolated.
**Payoff:** Low. **Effort:** Trivial.

The XSUB prologue checks `if (UNLIKELY(!affix->infix)) rebuild_affix_data(...)`
twice. `rebuild_affix_data` always sets `affix->infix` (via
`infix_forward_create_manual`) and croaks on failure, so the second check is
unreachable. Removing it saves one branch per call.

---

### 4. Direct SV field writes for scalar returns
**Status:** Ideas only — not prototyped.
**Payoff:** Low-Medium. **Effort:** Moderate.

`sv_setiv(TARG, x)` / `sv_setnv(TARG, x)` are real function calls with
magic/readonly checks. For a freshly allocated `dXSTARG` the SV is known
clean; direct `SvIVX`/`SvNVX` writes plus flag manipulation could remove a PLT
call per invocation. Needs careful Perl-version guarding.

---

### 5. Coalesce the three `alloca` calls
**Status:** Implemented in scratch worktree `affix-opt-235` (with #2 and #3).
Combined A/B above; falls back to the original per-buffer paths when any
buffer exceeds 2 KiB or `ret_align > 16` (correctness fix applied after the
benchmark — all benchmark types have alignment <= 16, so the A/B remains
valid). Full suite passes. **Cross-platform CI validation required.**
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
