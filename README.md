# ReSkate DAZ guard: fixing skate.'s endless respawn under FEX (ARM)

Fixes the **"skater falls through the map / disappears and respawns forever"** bug when skate. (with ReSkate)
runs on ARM devices through Proton ARM64 + **FEX** (tested on an AYN Thor running ArmadaOS).

- **Status:** working. With the patched DLL, bailing, respawning and teleporting all work on the Thor (2026-11-15).
- **Builds:** ReSkate tags `20261116` (current, recommended) and `20261114`, both for skate. Steam build `25414733`, plus this patch.
- **On x86 PCs:** no change in behaviour (see "Why it's safe on PC").

## Download
Don't build anything: grab the ready-made DLL from **[Releases](https://github.com/srsholmes/reskate-thor-dazfix/releases)**.

- **Latest ReSkate (20261116):** [v2-dazguard-20261116](https://github.com/srsholmes/reskate-thor-dazfix/releases/tag/v2-dazguard-20261116)
  (recommended; this is what the launcher installs today)
- **Older ReSkate (20261114 / 2.1.4-4 launcher):** [v1-dazguard-20261114](https://github.com/srsholmes/reskate-thor-dazfix/releases/tag/v1-dazguard-20261114)

Each release has the DLL and a zip containing `ReSkate.dll`, `README.txt` with install steps, `SHA256.txt` and the
source patch. Install instructions are in the release notes and below.

## Contents

| Path | What |
|---|---|
| [Releases](https://github.com/srsholmes/reskate-thor-dazfix/releases) | The patched `ReSkate.dll` builds and tester zips (not in the git tree) |
| `patch/0001-physics-daz-guard.patch` | Source of the fix; applies cleanly to ReSkate tags `20261114` and `20261116` |
| `docs/investigation.md` | Full write-up: symptom, causal chain, root cause in FEX/Wine, what was ruled out |
| `docs/thor-setup.md` | **Thor setup guide:** launch options, in-game settings for full speed, troubleshooting |
| `docs/daz-guard-sites.txt` | The 61 patched instructions in Skate.exe |
| `tools/daz-repro/` | Small x64 test of MXCSR DAZ/FTZ behaviour (Windows reference output included) |

## Install

1. Download the zip for your ReSkate version from [Releases](https://github.com/srsholmes/reskate-thor-dazfix/releases).
2. Close skate. and the ReSkate launcher.
3. In your ReSkate folder (the one with `ReSkateLauncher.exe` and `ReSkate.dll`), rename `ReSkate.dll` to
   `ReSkate.dll.original`.
4. Copy the `ReSkate.dll` from the zip into that folder.
5. Stop the launcher replacing it: turn **auto-update off** in the launcher settings, and/or add `--no-update` to the
   Steam launch options. Skip any update prompt.
6. Play. Optional check: `logs/ReSkate.log` should contain
   `Physics DAZ guard: 61 of 61 solver zero-guards treat denormals as zero.`

**Undo:** delete `ReSkate.dll` and rename `ReSkate.dll.original` back. You can also set `RESKATE_DAZ_GUARD=0` to
switch the guard off without swapping files.

**Requirements:** skate. Steam build 25414733 with the matching ReSkate release. If the game code doesn't match,
the byte checks fail and nothing is patched: it logs a warning and the game runs as stock.

**If bails never happen at all:** check that ReSkate's **No Bail** cheat is off (`nobail 0` in the console).

## What was wrong (short version)

1. skate. turns on the CPU's **flush-to-zero / denormals-are-zero** mode (FTZ/DAZ) with `_controlfp_s(_DN_FLUSH)`
   from `ucrtbase.dll`. On x86, any "absurdly tiny" (denormal) float is then treated as exactly 0.
2. Under Wine/Proton ARM64EC, that `ucrtbase` call runs as **native ARM64** code. It writes the ARM FPCR register
   directly (via `LdrpSetX64Information`), but **FEX keeps its own emulated MXCSR**, which stays at `0x1f80`.
   The game reads MXCSR back as "no flushing", saves that per job, and its job dispatcher reloads it (`vldmxcsr`).
   So the physics runs **without DAZ/FTZ** on the Thor.
3. The physics solvers guard normalisation like this: `inv = sqrt(1/x); inv &= (0 < x)`. With DAZ a denormal `x`
   counts as 0 and the guard zeroes the result. Without DAZ, a denormal `x` passes, `1/x` overflows to **+inf**,
   `inf × 0` gives **NaN**, and the whole rigid body (position, rotation, velocities) turns to NaN in one step.
   This mostly happens when the skater is nearly at rest.
4. The game's "fell out of the world" check (`vcomiss y, killY`) can't compare NaN, so it treats you as fallen and
   teleports you to the spawn. The new body goes NaN again, and the loop never ends.

## The fix

ReSkate patches the **61** zero-guards of this pattern in the physics solvers (`0x25d0000..0x2700000`). Each
`vcmpltps mask, zero, x` becomes a jump to a small stub, `vcmpps mask, x, [FLT_MIN], GE_OQ`, so the mask is
`x >= FLT_MIN`.

- **Safety:** every site's original bytes are checked first. If any differ, nothing is patched.
- **Bookkeeping:** stubs are allocated near the image and the patches are recorded with ReSkate's usual
  `patch_code_jump`.

### Why it's safe on PC
On x86 with DAZ on (how skate. always runs on Windows), a denormal compares as 0, so `0 < x` and `x >= FLT_MIN`
give bit-for-bit the same mask for every input. Only an emulator that ignores DAZ sees a difference, and there the
patch restores the x86 behaviour.

## The proper upstream fix (not done yet)
- **FEX (ARM64EC):** implement `Get/SetX64Information`, or sync `CPUState.mxcsr` with FPCR when native code changes
  it, so a native `_controlfp_s` and the emulated `stmxcsr`/`ldmxcsr` agree. Also map **DAZ** to FPCR.FZ on CPUs
  without FEAT_AFP. Today only FTZ is mapped there, so DAZ without FTZ (`0x1fc0`) is lost.
- **ReSkate:** optionally ship this guard (it's harmless on x86).
- **Side findings for FEX:** the default NaN for invalid ops is `0x7FC00000` (x86 gives `0xFFC00000`), and
  `vfmsub231ps` flips the sign of a NaN accumulator.

Nothing has been reported upstream yet.

## Thor setup guide (speed + settings)
Slow motion on the Thor is a separate issue from this bug: skate.'s fixed-rate physics can't keep up with the CPU
cost of translating x86 code, so the game runs slower than real time. **[docs/thor-setup.md](docs/thor-setup.md)** is
the full guide:
- the launch options and environment variables (which are needed and which are optional)
- **Simulation rate 30 Hz**, the setting that gets the game to real speed
- the graphics settings that look good and still hold 30 fps
- the Performance power profile, and troubleshooting

## Licence
GPL-3.0 (see `LICENSE`), the same as ReSkate. ReSkate is GPL-3.0. The patched DLLs are built from upstream ReSkate tags `20261114`/`20261116` plus `patch/0001-physics-daz-guard.patch`
(the complete source of the change). Upstream: https://github.com/Dingo-Shenanigans/ReSkate
