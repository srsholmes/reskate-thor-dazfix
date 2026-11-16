# AYN Thor (ArmadaOS) launch setup for skate. + ReSkate

This is the setup that gives the best speed (from 2026-11-13), with the DAZ guard DLL. It was checked on the live
process on 2026-11-15: `FEX_TSOENABLED=0`, cores 3-7, Performance profile.

## Environment
File `~/gametest/skate/env.json`, loaded by the 3354750 block of `Proton 11.0 (ARM64)/user_settings.py`:

```
FEX_TSOENABLED=0                      # biggest CPU saving
tu_enable_ssbo_emulation=true         # Turnip: skater renders
tu_enable_texel_buffer_emulation=true
VKD3D_FAKE_TILED=1                    # ground renders (patched vkd3d)
VKD3D_FAKE_MSAA8=1
VKD3D_FEATURE_LEVEL=12_1
WINEDLLOVERRIDES=dxgi,d3d12,d3d12core=n,b
```
Don't use extra FEX app configs (TSO/VectorTSO/Multiblock overrides). None of them fix the respawn bug, and they
cost speed.

## Files in the ReSkate folder
- Patched vkd3d `d3d12.dll`/`d3d12core.dll`, Proton's x64 DXVK `dxgi.dll`.
- `ReSkate.dll` = `ReSkate-dazguard-20261116.dll` with the 20261116 launcher (earlier: the 20261114 build on the 2.1.4-4 launcher).

## Launch option
```
armada-game-launch /usr/bin/taskset -c 3-7 /usr/bin/bash -c 'exec "${@/*EAAntiCheat.GameServiceLauncher.exe//var/home/armada/Games/ReSkate/ReSkateLauncher.exe}"' -- %command% --no-gui --no-update --offline
```
`taskset` must come after the wrapper, because the wrapper resets the CPU affinity.

## Device
**Performance** power profile (Armada Control). Eco caps the CPU at ~1.5-1.8 GHz and the GPU at 475 MHz and brings the
slow motion back.

## In-game settings (2026-11-16, full speed + better visuals)
skate. saves these itself (Settings → Graphics), so they survive restarts.

| Setting | Value | Why |
|---|---|---|
| **Simulation rate** | **30 Hz** | At 60 Hz the Thor manages ~58 ticks/s even standing still (0.90-0.97× speed, "floaty"). At 30 Hz: 1.00× real time while skating. |
| Frame rate target | 30 fps, VSync on | |
| Resolution | 1280×720 fullscreen (Game Mode max), HDR off | |
| Dynamic resolution | on, 70-100% | |
| Upscaling / frame generation | off | |
| Anti-aliasing | Temporal (TAA) | |
| Texture filtering | Ultra | |
| Texture quality | Medium | High risks streaming hitches: RAM is ~14.3 of 15.5 GB plus ~4 GB compressed swap |
| Lighting | Medium | |
| Shadows | Medium | |
| GI grid | Medium | |
| Ambient occlusion | SSAO | |
| World detail, mesh, effects, post-processing | Low | World detail and mesh add CPU work, and the CPU is what limits speed |
| Depth of field, motion blur | off | |
| Camera Y axis (Controls) | inverted | Simon's preference |

Measured standing still at the Hedgemont sign with these settings: 30 fps, a client (CPU) frame of ~16-17 ms out of
the 33 ms budget, and the GPU ~40% busy at 680 MHz.

Notes:
- The limit is the CPU (x86 code translated by FEX), not the GPU. GPU-only settings are close to free; anything that adds
  draw calls or simulation work brings the slow motion back.
- Under 33 ms the game doesn't catch up: if the client frame runs over its budget, game time runs slower than real time.
- 45 Hz physics also ran at full speed (18 ms of a 22 ms budget), but only via the console
  (`SimulationTime.ForceSimRate 45` + `SimulationTime.MaxSimFps 45`, then bail or teleport), and it's lost on restart.
  The menu only offers 30 or 60.
- A physics rate change only applies to the skater after a respawn or teleport.

## Gotchas
- A Proton update wipes `user_settings.py`, and with it the env.
- Debugging sessions have left Balanced power and slow FEX configs behind. Re-check after any test session.

## 2026-11-16 update
- Thor on ReSkate **20261116** + dazguard rebuilt for that tag (SHA `f253581c…`, release v2); `DAZ guard: 61 of 61`, no respawn loop.
- `--no-gui` back in the launch option (since 2026-11-16, the launch option ends `%command% --no-gui --no-update --offline`); `--no-update` kept and launcher auto-update off. Skip update prompts until the
  patch is rebuilt for the new tag (the patch applied cleanly to 20261116).
- Debug leftovers to check after test sessions: ReSkate `nobail` (was left at 1 → flatground tricks never bailed),
  `noclip`, `darkpop` should all be 0; power profile Performance; no extra FEX app config.
