# Ryzen Z1 / Second Son optimization research — 11 October 2026

This source review was completed before creating the experimental branch.
Baseline: `amromnia/shadPS4-Infamous-FixBuild`,
`perf/ally-z1-integrated`, commit `3f49a5c0`.
The objective is lower overhead and steadier frame times on the regular Z1,
with the existing OOM, input, motion, touchscreen and correctness fixes retained.
No performance increase has yet been measured on the Ally.

## What the supplied log establishes

`rog_logs/new_crash/shad_log (1).txt` runs `d22bdbd0`, CUSA00004 version 1.07,
precise readbacks, DMA enabled, threaded recording and upload threading enabled,
async compute disabled, and a 1280x720 rendering patch. At 23–24 FPS the recorded
GPU busy time is about 30–31 ms/frame and the command-processing thread is
97–99% busy. In those intervals game threads accumulate 2.6–3.1 seconds of
readback waits per ten seconds; these are overlapping thread timings.
Many write faults and protection changes are recorded. They are emulator
tracking faults, not evidence of Windows hard paging. Some stalls include shader
compilation, while others do not. The terminal error is Vulkan device loss;
there is not enough evidence to attribute it to SaveData or fix it safely.

## Architecture and compilation

[AMD's Z1 specification](https://www.amd.com/en/products/processors/handhelds/ryzen-z-series/z1-series/z1.html)
lists six cores/twelve threads, two Zen 4 and four Zen 4c cores, and four RDNA 3
graphics compute units. Shared memory and a small GPU make wasted transfers and
serialization especially relevant. Overall CPU utilization does not show whether
one critical emulator thread is saturated. Do not hard-code Windows CPU numbers:
logical-processor topology and scheduling must be detected before affinity tuning.

[Clang's manual](https://clang.llvm.org/docs/UsersManual.html) and
[ThinLTO documentation](https://clang.llvm.org/docs/ThinLTO.html) support a speed
optimized Release build with ThinLTO. Use O3 and Zen 4 scheduling (`-mtune=znver4`),
retaining x86-64-v3 instructions so build-time generator tools and the binary
also run on this Intel build PC. `-march=native` would optimize for the wrong PC.
Do not blanket-enable fast-math/Ofast: host floating-point and shader handling
have correctness requirements. PGO is a later candidate, requiring representative
gameplay traces rather than an invented profile. ThinLTO adds build/link cost;
compiler flags alone do not demonstrate an FPS gain.

## shadPS4 sources inspected

| Source / pinned revision | Findings and disposition |
| --- | --- |
| [Official upstream](https://github.com/shadps4-emu/shadPS4/commits/main/), `3a9f57265d` | Reviewed current commits through IMAGE_GATHER4_C_L, fp64 detection, GNM compute wait, clamped math and indirect draw APIs. The earlier Z1 branch already backports substantial memory/renderer corrections. New service or shader functionality is not evidence of a Z1 performance benefit; avoid an indiscriminate upstream merge. |
| [Original FixBuild](https://github.com/rayhlancaner-png/shadPS4-Infamous-FixBuild), `9eb93b9694` | Parent baseline includes second-queue compute, independent recording/upload threads, caching and predictive readbacks. Our branch retains them. Its performance counters, stack sampler and GPU timing queries run separately from launcher Vulkan diagnostic switches. |
| [AYOUB original](https://github.com/weshayoub92/shadPS4-AYOUB1080p), `153a5aa7f6`, also mirrored by [T-Damer](https://github.com/T-Damer/shadPS4-AYOUB1080p) | Read README, tuning header and compared memory/renderer source. A single large change based on upstream `2338a06`, not a compatible series of patches. Threading, lookup caching, offloaded/prefetched readbacks overlap our code. Extra hot-page hashing, selective write discard/merge and game-profile clock changes depend on a different tracking design. Its headline tests use a 9950X3D, not a Z1. Pipeline skipping can omit game work, so do not transplant its complete profile. |
| [GR2fork EXODUS](https://github.com/junminlee2004/GR2fork), `fc597690e0` | Inspected recent 100 commits and readback-offload implementation (`b43bdc8bac`). Offloading waits to the faulting thread is already present in our fork. Its broader hacks and linear-image paths target Gravity Rush. The latest within-wave barrier change overlaps our earlier shader barrier backport. No new readback-offload transplant is needed. |
| [MeWeGo FixBuild](https://github.com/MeWeGo/shadPS4-Infamous-FixBuild), `934b988ecb` | Inspected recent 35 commits and exact patches `98b92a9289` and `3452bc1da7`: extend sequential invalidation from 16 to 64 pages and widen demand readbacks from 512 KiB to 1 MiB. These are useful experimental tunings, not a proven Z1 result. Its newest async-compute batching is explicitly disabled by default because batches cannot see later uploads; do not import that path. Narrower barriers and earlier readback queue dependencies need a full hazard audit, not an unconditional stage-mask edit. |
| [TFSG28 releases](https://github.com/TFSG28/shadPS4-Infamous-FixBuild/releases) | Another published FixBuild lineage; release descriptions alone do not establish distinct applicable code or performance on the Z1. |
| [Bloodborne DLSS/FSR PR #16](https://github.com/IFreemz/shadPS4-Bloodborne-DLSS-FSR/pull/16), `e172285df8` | Reviewed PR description, changed-file list and memory/cache diffs. Tested by author on Intel/RTX 3060, Bloodborne. Velocity mirrors and upscaling hooks are game specific. A useful general idea is extending CPU write faults only to non-GPU-dirty pages under the tracker lock. Our coarse invalidate-ahead currently rejects an entire window if any GPU page is dirty; adapt this idea to our existing tracker and test dirty-data preservation. Its read-only clean-binding fast path already exists in our word-level summaries. Cache rebuild patch responds to a size mismatch not observed in this log. |

The review uses code and pinned commits, not desktop FPS claims as promises for
the handheld. No claim that every fork on GitHub has been inspected is made.

## Other PS4 emulators and compatibility layers

Primary repository README and current repository metadata were inspected:

* [fpPS4](https://github.com/red-prig/fpPS4), pushed 9 October 2026: Free Pascal
  compatibility layer, currently rewriting its core in `kern`. Its README says
  large AAA games still do not work. Different implementation, no demonstrated
  Second Son/Z1 replacement or directly portable memory-cache patch.
* [RPCSX](https://github.com/RPCSX/rpcsx), pushed 29 June 2026: experimental
  PS4/PS5 emulator for Linux. Different kernel/renderer architecture and not a
  drop-in Windows solution.
* [Obliteration](https://github.com/obhq/obliteration), pushed 26 September 2026:
  kernel/VMM approach; current README explicitly says it cannot run games yet.
* [psOff public](https://github.com/SysRay/psOff_public): archived public source;
  README points to the replacement closed-source premium project. No accessible
  current premium renderer source to borrow.
* [GPCS4](https://github.com/Inori/GPCS4), last push August 2023, and
  [original Kyty](https://github.com/InoriRus/Kyty), last push May 2024:
  older experimental implementations, no verified Second Son optimization here.
* [Orbital](https://github.com/AlexAltea/orbital), last push April 2024:
  experimental low-level approach, unsuitable as a drop-in shadPS4 renderer.
* [Spine preview](https://github.com/devofspine/spine), last push May 2022:
  binary preview without usable renderer source to port.
* [KytyPS5](https://github.com/KytyPS5/KytyPS5), pushed 11 October 2026:
  actively developed PS5 translation layer. Its PS5 work does not establish
  compatibility with Second Son's PS4 executable or provide a plug-in layer
  for shadPS4. No transplant was selected.
* [SharpEmu](https://github.com/sharpemu/sharpemu): current C# PS5 project,
  latest inspected release `v0.0.5-nexus-release.2` (`47d4c07`). Its README
  explicitly excludes PS4 emulation and points to shadPS4 for that platform.
  Background resource-plan compilation is conceptually relevant but not a
  compatible patch for this C++ renderer.

Some search results advertise implausibly broad PS4/PS5 compatibility; only the
original project documentation and source were used for technical decisions.

## Selected experiment and boundaries

1. Build-time switch defaults detailed performance profiling OFF, including
   CPU counters/timers, thread stack sampling, GPU timing queries, fault-history
   diagnostics and readback statistics. Keep the minimal frame counter used
   by rewrite tracking, plus normal crash/error logging and touchpad diagnostics.
2. O3, ThinLTO, Zen 4 scheduling; bound local LTO worker count to avoid memory
   exhaustion during compilation. Keep a separate output directory.
3. Extend sequential write-fault lookahead to at most 64 pages, and mark only
   non-GPU-dirty pages under the region lock. Preserve texture watchers and
   dirty GPU pages. This is an optimization of the existing path, not disabling
   accurate memory tracking.
4. Make readback-window size a build parameter, with the experimental Z1 branch
   testing 1 MiB and a 512 KiB comparison available. Retain precise dirty-range
   copying, arena boundaries, stale-copy recovery, timeline retirement and
   existing adaptive prefetch backoff. Wider windows may copy more data and lose
   performance on shared memory, so benchmark rather than claim improvement.
5. Do not turn on unverified asynchronous-compute routes, skip readbacks,
   drop rendering for FPS, remove barriers, force thread affinity or modify
   saving on the current evidence. Keep previous OOM fixes.

## Validation needed

Run tracker tests that protect mixed CPU/GPU words, word boundaries, preservation
of GPU dirtiness and generation updates, plus existing touchscreen tests. Compile
the real Windows Release target with the chosen optimization flags. Verify the
resulting command lines and absence of diagnostic symbols/query setup where
profiling is OFF. A diagnostic build must still compile.

Ally validation remains external: same game version, save, 720p patch, 6 GiB
allocation and power mode; warmed shader cache; repeat traversal/combat with
overlay FPS/frame-time data and monitor RAM/VRAM. Compare baseline, profiling-off,
and full experiment. Compare 512 KiB versus 1 MiB if full experiment regresses.
An hour of play and repeated saves/input gestures are needed to assess stability.
Compile success and host-side tracker tests cannot prove GPU or gameplay correctness.
