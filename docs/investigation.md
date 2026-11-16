# Investigation: skate.'s endless respawn under FEX (ARM64)

## Symptom
On an ARM handheld (AYN Thor, ArmadaOS, Proton ARM64 + FEX), after any teleport or respawn, and sometimes after the
skater comes to rest, the skater vanishes and is recreated about every 0.5 s forever, leaving an empty screen.
x86 Windows and x86 Proton are unaffected.

## Causal chain
All addresses are Skate.exe RVAs for Steam build 25414733.

1. **The game turns on FTZ+DAZ** on its threads with `_controlfp_s(0x0108001f, 0x0308031f)` (includes `_DN_FLUSH`),
   imported from `ucrtbase.dll`. Its job dispatcher also reloads a per-job MXCSR captured earlier
   (`vldmxcsr` at `0x138a79f` and `0x138abe3`). On Windows its threads run with MXCSR `0x9ff1`/`0x9ffd`
   (FTZ=1 DAZ=1). Under FEX every thread reads `0x1f80` (FTZ=0 DAZ=0).
2. **The physics solvers guard their normalisations with eps = 0:**
   `inv = sqrt(1/x)` (`vdivps`/`vsqrtps`), then `mask = 0 < x` (`vcmpltps` against a `vxorps` zero), then `inv &= mask`.
   With DAZ, a denormal `x` compares as 0 and the result is zeroed. Without DAZ, a denormal `x` (~1e-39, from tiny
   contact or friction velocities on a resting body) passes the guard, `1/x` overflows to +inf, and inf × 0 gives NaN.
3. **The integrator (`0x262d960`) spreads the NaN** to the body's position, orientation and velocities in one step.
4. **The write-back (`0x2b7ad20`) copies the NaN pose** from the physics engine into the entity transform.
5. **The fell-out-of-world check (`0x554790`, every 0.5 s) misfires.** It does `vcomiss y, killY; jae ok`, and NaN
   compares as unordered, so it requests a teleport to spawn (`requestTeleport 0x565010`, called from `0x5548ba`).
   The skater is rebuilt, its body goes NaN again, and the loop never ends.

## Root cause in the emulation stack
Sources: FEX `ea43314`, Wine master.

- **Wine:** Wine's ARM64EC CRT (`dlls/msvcrt/math.c`) implements `_controlfp_s` on `__arm64ec__` by calling
  `__os_arm64x_set_x64_information(0, mxcsr)`. Without an emulator override, ntdll's `LdrpSetX64Information`
  (`dlls/ntdll/signal_arm64ec.c`) converts MXCSR to the **host FPCR/FPSR** (`mxcsr_to_fpcsr`, `dlls/ntdll/unwind.h`)
  and writes them directly.
- **FEX:** FEX keeps its own emulated MXCSR in `CPUState.mxcsr`. `STMXCSR` returns that field (`GetMXCSR`), and
  `LDMXCSR` writes it and reprograms FPCR (`RestoreMXCSRState` → `SetRoundingMode`: rounding and FTZ→FPCR.FZ;
  DAZ→FPCR.FIZ only on FEAT_AFP hosts).
- **The result:** the native `_controlfp_s` never updates FEX's emulated MXCSR. The x64 game code reads back
  `0x1f80`, stores it in its job contexts, and the dispatcher's `vldmxcsr 0x1f80` reprograms FPCR without flushing.
  Physics then runs without FTZ/DAZ.
- **The repro confirms it:** a test built with the **static** CRT (its own x64 `ldmxcsr`) behaves exactly like
  Windows under FEX. The desync only happens through the DLL CRT, which runs as native ARM64 code. A faithful repro
  must call `ucrtbase.dll`'s `_controlfp_s` (`/MD`, or `GetProcAddress`).
- **A second gap:** on hosts without FEAT_AFP, DAZ without FTZ (`0x1fc0`) isn't honoured, because only FTZ maps to
  FPCR.FZ. ARM's FZ flushes both inputs and outputs, so FTZ+DAZ together do work.

### Upstream fix candidates
- **FEX ARM64EC:** provide Get/SetX64Information, or sync `CPUState.mxcsr` from FPCR when native code changes it,
  so a native `_controlfp_s` and the emulated `stmxcsr`/`ldmxcsr` agree.
- **FEX on non-AFP hosts:** map MXCSR.DAZ to FPCR.FZ as well (an approximation).
- **Wine:** check that `mxcsr_to_fpcsr` mapping DAZ to FPCR bit 19 is intended.

## Workaround (this repo)
`patch/0001-physics-daz-guard.patch` rewrites the 61 zero-guards of this pattern in `0x25d0000..0x2700000`
(listed in `daz-guard-sites.txt`) to `x >= FLT_MIN` (`vcmpps … GE_OQ` against FLT_MIN). That's identical to x86
with DAZ on, and fixes the loop under FEX.

## What was ruled out along the way
- CPU speed. The game was tested held to 1–2 cores on x86.
- Proton itself. x86 Proton is fine.
- ReSkate's physics pool and wake-pass tweaks.
- FEX settings: TSO, VectorTSO, MemcpySetTSO, HalfBarrierTSO, Multiblock, X87ReducedPrecision, SMCChecks, AVX off.
- FEX's min/max NaN ordering. It's correct on non-AFP hosts.
- The integrator's rotation block. It's bit-exact under FEX when replayed on 1,304 recorded inputs.

## Other FEX vs x86 differences found
- The default NaN for invalid operations (0/0, 0·inf, inf−inf, sqrt(−1)) is `0x7FC00000`; x86 gives `0xFFC00000`.
- `vfmsub231ps` with a NaN accumulator flips the NaN's sign.

## Method
The bug was located step by step:
1. Diagnostic ReSkate builds (see `diagnostics/`) logged the respawn caller stacks, the TeleportManager state, the
   kill-check inputs, the physics pose and the solver inputs.
2. Hardware write watchpoints on Windows identified the writers of the body state.
3. Comparing Windows and FEX runs value by value narrowed it down to the solver's zero-guards.
