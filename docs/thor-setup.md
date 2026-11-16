# skate. + ReSkate on the AYN Thor (ArmadaOS): setup guide

How to get skate. running at full speed on the Thor, with the DAZ guard fix from this repo. Tested on an AYN Thor
(Snapdragon 8 Gen 2) running ArmadaOS with Proton 11.0 (ARM64), skate. build 25414733 and ReSkate 20261116.

There are three parts:
1. **Launch options:** environment variables and flags.
2. **In-game settings**, the most important being Simulation rate 30 Hz.
3. **Device settings:** the Performance power profile.

## 1. Launch options

Steam → skate. → Properties → **Launch Options**. Put the environment variables **before** everything else:

```
FEX_TSOENABLED=0 tu_enable_ssbo_emulation=true tu_enable_texel_buffer_emulation=true %command% --no-update
```

If you already use a wrapper (e.g. `armada-game-launch …`) or other ReSkate flags, keep them and put only the
variables in front.

| Variable | What it does | Needed? |
|---|---|---|
| `tu_enable_ssbo_emulation=true`<br>`tu_enable_texel_buffer_emulation=true` | Tells the Thor's GPU driver (Turnip) to work around its 128 MB buffer limit. Without them the skater renders with missing head and arms. | **Yes** |
| `FEX_TSOENABLED=0` | Turns off FEX's emulation of x86 memory ordering. The biggest CPU saving we found, with no problems in skate. | Strongly recommended |
| `VKD3D_FAKE_TILED=1`<br>`VKD3D_FAKE_MSAA8=1`<br>`VKD3D_FEATURE_LEVEL=12_1`<br>`WINEDLLOVERRIDES=dxgi,d3d12,d3d12core=n,b` | Only for a **patched vkd3d** build (`d3d12.dll`/`d3d12core.dll` in the game folder) that fixes a white/missing ground on some spawns. They do nothing with stock vkd3d. | Only with patched vkd3d |

ReSkate flags:
- `--no-update` stops the launcher replacing the patched `ReSkate.dll`. Also turn auto-update off in the launcher.
- `--no-gui` (optional) skips the launcher window and starts the game directly.
- `--offline` (optional) plays offline.

Don't add extra FEX app configs (TSO / VectorTSO / Multiblock overrides). They don't fix anything for skate.
and they cost speed.

**Optional: big cores.** Pinning the game to the faster cores helped a little in our tests. Put `taskset -c 3-7`
**after** any wrapper, because wrappers can reset the CPU affinity:
`armada-game-launch /usr/bin/taskset -c 3-7 … %command%`.

## 2. In-game settings

All of these are in **skate.'s own Settings → Graphics** menu, not the ReSkate mod menu. The game saves them, so you
only set them once.

| Setting | Value | Why |
|---|---|---|
| **Simulation rate** | **30 Hz** | The most important one. See below. |
| Frame rate | 30 fps, VSync on | |
| Resolution | 1280×720 fullscreen | The highest Game Mode offers |
| Dynamic resolution | on, 70-100% | |
| Upscaling / frame generation | off | |
| Anti-aliasing | Temporal (TAA) | |
| Texture filtering | Ultra | Cheap, much sharper ground |
| Texture quality | Medium | High fills the RAM (14+ of 15.5 GB) and causes stalls |
| Lighting, shadows, GI grid | Medium | |
| Ambient occlusion | SSAO | |
| World detail, mesh, effects, post-processing | Low | These add CPU work, and the CPU is the limit |
| Depth of field, motion blur | off | |

With these settings the game holds 30 fps at real speed. The CPU uses about 16-17 ms of each 33 ms frame, and the
GPU is about 40% busy.

### Why Simulation rate 30 Hz
skate. runs its physics at a fixed rate (60 steps a second by default) and **does not catch up when it falls behind**.
If the CPU can't finish the steps in time, the whole game runs in slow motion, even though the fps counter still
says 30. On the Thor, x86 code is translated to ARM by FEX, so the CPU is the bottleneck:

| Simulation rate | Steps/s achieved | Game speed |
|---|---|---|
| 60 Hz | ~58 standing still, lower while skating | 0.90-0.97×, "floaty" slow motion |
| 30 Hz | 30 | **1.00×**, real time (measured while skating) |

After days at 60 Hz, 30 Hz can feel "too fast" at first. That's real speed.

If you change the rate from the console instead (`SimulationTime.ForceSimRate` + `SimulationTime.MaxSimFps`, same
value), it only applies after a bail, respawn or teleport, and it's lost on restart. 45 Hz also ran at full speed
in our tests, but it's console only. The menu offers 30 or 60.

## 3. Device settings

- **Power profile: Performance.** Eco caps the CPU at about 1.5-1.8 GHz (instead of 2.0-3.0) and the GPU at 475 MHz
  (instead of 680), and that brings the slow motion straight back.
- Keep the fan curve from being too quiet: the fastest core runs at 80-94 °C in skate.

## Troubleshooting

**Slow, floaty gameplay:** check that Simulation rate is 30 Hz and the power profile is Performance.

**Power drops (e.g. 13 W → 4 W) with big fps drops:** the chip is being held back or is waiting. Check, in order:
1. The power profile (Performance, not Eco/Balanced).
2. Heat: the CPU throttles hard when it gets too hot.
3. Memory: texture quality above Medium fills the RAM, and the game stalls while it swaps.

MangoHud with CPU/GPU clocks and temperatures, captured during a drop, shows which one it is.

**Skater falls through the map / respawns forever:** that's the FEX DAZ bug. Install the DAZ guard `ReSkate.dll`
from [Releases](https://github.com/srsholmes/reskate-thor-dazfix/releases) (see the README). The log should show
`DAZ guard: 61 of 61`.

**Bails never happen:** ReSkate's No Bail cheat is on. Set `nobail 0` in the console.

## Appendix: the maintainer's exact Thor setup

For reference, this is the setup we tested on. It loads the environment variables from a file instead of the launch
option.

- `Proton 11.0 (ARM64)/user_settings.py` has a block for app 3354750 that loads `~/gametest/skate/env.json` with
  all seven variables above. **A Proton update replaces `user_settings.py`**, so the launch-option method is safer.
- The patched vkd3d `d3d12.dll`/`d3d12core.dll` and Proton's x64 DXVK `dxgi.dll` are in the ReSkate folder.
- `ReSkate.dll` is `ReSkate-dazguard-20261116.dll` (release v2) with the 20261116 launcher.
- Launch option (the `bash -c` part swaps EA's anti-cheat launcher for `ReSkateLauncher.exe`; adjust the path to
  your own ReSkate folder):

```
armada-game-launch /usr/bin/taskset -c 3-7 /usr/bin/bash -c 'exec "${@/*EAAntiCheat.GameServiceLauncher.exe//var/home/armada/Games/ReSkate/ReSkateLauncher.exe}"' -- %command% --no-gui --no-update --offline
```

- After test sessions, check that `nobail`, `noclip` and `darkpop` are 0, the power profile is Performance and no
  extra FEX app config is left behind.
