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
- `ReSkate.dll` = `ReSkate-dazguard-20261114.dll` (stock kept as `ReSkate.dll.stock-2.1.4-4`).

## Launch option
```
armada-game-launch /usr/bin/taskset -c 3-7 /usr/bin/bash -c 'exec "${@/*EAAntiCheat.GameServiceLauncher.exe//var/home/armada/Games/ReSkate/ReSkateLauncher.exe}"' -- %command% --no-update --offline
```
`taskset` must come after the wrapper, because the wrapper resets the CPU affinity.

## Device
Performance power profile, all graphics Low, 30 fps cap, sim rate default 60 (no ForceSimRate/MaxSimFps).

## Gotchas
- A Proton update wipes `user_settings.py`, and with it the env.
- Debugging sessions have left Balanced power and slow FEX configs behind. Re-check after any test session.

## 2026-11-16 update
- Thor on ReSkate **20261116** + dazguard rebuilt for that tag (`f253581c…`); `DAZ guard: 61 of 61`, no respawn loop.
- `--no-gui` removed (launcher shows); `--no-update` kept and launcher auto-update off. Skip update prompts until the
  patch is rebuilt for the new tag (the patch applied cleanly to 20261116).
- Debug leftovers to check after test sessions: ReSkate `nobail` (was left at 1 → flatground tricks never bailed),
  `noclip`, `darkpop` should all be 0; power profile Performance; no extra FEX app config.
