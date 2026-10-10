# Ally Z1 / Second Son integration candidate

This experimental branch starts at the exact successful Windows build,
`ba072e87061669b219116b3f081e8658a95bd047` (`fix/sparse-backing-oom`).
The official upstream history reviewed ends at
`811bfcd7d17ed8ae975c477d9ff28abc744e1ca8` on 2026-10-10. The parent
Infamous fork still ends at release 1.0.16, `9eb93b96`.

The target is a Windows ROG Ally Z1 with 16 GB shared RAM. Compilation and
gameplay validation are separate: no FPS improvement or crash-free gameplay
is established by assembling these patches. Keep the original build for comparison.

## Preserved baseline

All baseline commits remain ancestors of this branch, including experimental
sparse backing OOM recovery, device-address allocation flags, bounded physical
allocations, small-budget residency granularity, exact-size retries, and compatible
non-device-local fallback. Unified-memory devices may have no separate fallback heap.

The fork's threaded recording, asynchronous uploads, large-read synchronization,
readback cache, shader persistence, and optional compute queue remain intact.
Windows-only workflows still require manual dispatch.

## Official upstream backports

Each backport commit records its original SHA using `cherry-pick -x`.
Conflicts were adapted to preserve the fork's implementation rather than replacing
entire files with upstream versions.

| PR | Change | Expected relevance |
| --- | --- | --- |
| [5231](https://github.com/shadps4-emu/shadPS4/pull/5231) | Presentation waits and fragment texture access | Correct synchronization; uses the fork's per-wait stage API |
| [5257](https://github.com/shadps4-emu/shadPS4/pull/5257) | Shader NaN and legacy arithmetic handling; Sirit dependency | Correct generated shaders |
| [5260](https://github.com/shadps4-emu/shadPS4/pull/5260) | Instance-ID unbiasing | Shader correctness |
| [5261](https://github.com/shadps4-emu/shadPS4/pull/5261) | D32/D32S8 buffer copies | Depth copy correctness |
| [5262](https://github.com/shadps4-emu/shadPS4/pull/5262) | Vertex/index/indirect access tracking | Buffer hazards; preserves stream-buffer bypass and upload tracking |
| [5264](https://github.com/shadps4-emu/shadPS4/pull/5264) | Guest-requested early fragment testing | May avoid unnecessary fragment work; no forced early-Z |
| [5265](https://github.com/shadps4-emu/shadPS4/pull/5265) | More shader buffer descriptors | Avoid descriptor-capacity failures |
| [5272](https://github.com/shadps4-emu/shadPS4/pull/5272) | Restore GPU image-copy fast path | Avoid unnecessary copy overhead |
| [5274](https://github.com/shadps4-emu/shadPS4/pull/5274) | Step rates with base instance | Shader correctness |
| [5258](https://github.com/shadps4-emu/shadPS4/pull/5258) | Destroy nested command coroutine frames | Reduce leaked host memory during continued play |
| [5229](https://github.com/shadps4-emu/shadPS4/pull/5229) | Initialize expanded depth subresources | Texture correctness |
| [5197](https://github.com/shadps4-emu/shadPS4/pull/5197) | Zero/one MIN/MAX blend factors | Rendering correctness |
| [5220](https://github.com/shadps4-emu/shadPS4/pull/5220) | EXEC scope after empty branches | Shader correctness |
| [5283](https://github.com/shadps4-emu/shadPS4/pull/5283) | Apply fast clears during FMASK decompression | Rendering correctness |
| [5285](https://github.com/shadps4-emu/shadPS4/pull/5285) | Shared-memory subgroup barriers | Correct compute dependencies |
| [5278](https://github.com/shadps4-emu/shadPS4/pull/5278) | Preserve indirect index capacity and clamp mapped index ranges | Correct indirect draws; preserves stream upload optimization |
| [5296](https://github.com/shadps4-emu/shadPS4/pull/5296) | Associate copied stencil/depth planes | Correct depth copies; retains first-page lookup |
| [5299](https://github.com/shadps4-emu/shadPS4/pull/5299) | Copies across differing layer counts | Avoid unnecessary copy assertion |
| [5317](https://github.com/shadps4-emu/shadPS4/pull/5317) | Restore Windows protections after mapping splits | Preserve GPU tracking and guest protections |
| [5307](https://github.com/shadps4-emu/shadPS4/pull/5307) | Precomputed tiling layouts through 2048 pixels | Faster layout calculation for qualifying textures |
| [5327](https://github.com/shadps4-emu/shadPS4/pull/5327) | Null index buffer without mandatory maintenance6 | Driver compatibility |
| [5316](https://github.com/shadps4-emu/shadPS4/pull/5316) | Advance write source across sparse backing runs | Prevent backing-write data corruption |
| [5340](https://github.com/shadps4-emu/shadPS4/pull/5340) | Avoid unwanted analog smoothing | More responsive physical sticks/triggers |
| [5276](https://github.com/shadps4-emu/shadPS4/pull/5276) | Explicit fmt/cstring includes | Build compatibility |

## Additional unmerged candidates

- [5234](https://github.com/shadps4-emu/shadPS4/pull/5234),
  `b0adb63c43f50e6eff7d30e96414c1a62bc56b90`: select a sparse-capable queue.
  Adapted device creation to retain the optional second graphics-family queue;
  a different sparse family requests one queue only. A correctness fix, particularly
  relevant to drivers that separate sparse and graphics queue families.
- [5239](https://github.com/shadps4-emu/shadPS4/pull/5239), diff at
  `0a2e0a1c241617a0ab60cc1553a1777a51f01d22`: staged publication of runtime
  trampoline jumps. The author reports fewer Second Son loading crashes on Windows.
  This remains experimental self-modifying-code behavior, not proof that every
  possible concurrent instruction-fetch race is eliminated. AOT uses plain writes.
- [5282](https://github.com/shadps4-emu/shadPS4/pull/5282),
  `d13e2fd5d52134e93e0e082907d05d08d7fedc22`: retain existing image mip/layer
  resources during expansion. Tested by its author on a Z1 Extreme under Linux in
  UFC 4, not Second Son on a Windows Z1. Correct preservation may retain more memory.
- [5310](https://github.com/shadps4-emu/shadPS4/pull/5310),
  `be37d724`: storage-buffer dependencies across split 64-lane compute waves.
  Specifically reported to fix Second Son particle flicker on non-AMD GPUs.
  Conditional on host subgroup size, so it may not activate on the Ally.

## Integration fixes

- Added indirect-command reads to the fork runtime's read-access mask and tracked
  them at the draw-indirect stage. Otherwise the backported access classification
  would miss read hazards in this fork's tracking implementation.
- Retained stream-buffer bypass, bounded descriptor storage, and capacity for both
  indirect arguments and count buffers.
- Changed shader cache revision, shader binary version, and pipeline key version
  to avoid loading incompatible compiled shaders or changed fragment keys.
  The first run recompiles; warmed-cache runs are needed for performance comparison.

## Reviewed but not stacked

- 5233 device-address allocation: already included in our OOM patch.
- 5240 first-page image lookup, 5255 duplicate image synchronization, 5277 lock-free
  region bypass: the fork already has equivalent targeted optimizations. Replacing
  its implementation would risk losing generation/rewrite/upload tracking.
- 5336 slot-driven texture GC and 5213 raised image-view limits: the fork already
  collects for slot pressure; stacking another collector adds avoidable work.
- 5248 motion tilt/shake and 5238 touchpad emulation: existing fork functionality.
- 5215 wave-min folding: DMC5-specific evidence, new Sirit dependency, AMD untested.
- 5168 oversized workgroups: draft Minecraft fix, broader dispatch rewrite, no
  evidence of this failure in Second Son logs.
- 5218 resource guards: large experimental GT7 change without a demonstrated
  Second Son benefit; changes resource discovery and permutation behavior.
- 5280, 5120, 5121 raw image synchronization/copy: RE2 evidence and substantial
  overlap with the fork's optimized coherence paths; not combined without a
  Second Son reproduction showing stale raw-buffer image data.
- 5303 image resizing: UFC 4 evidence and a new allocation/copy path; no demonstrated
  Second Son need to justify extra allocation behavior on 16 GB.
- Unrelated networking/POSIX services, Linux-only patches, and Intel reciprocal
  emulation do not improve the Windows AMD Z1 target and were not backported.

## Validation and testing

Local review checks ancestry, patch diffs, conflict resolution, unchanged workflow
triggers/runners, submodule pinning, and whitespace. This PC has no local C++ build
toolchain; GitHub Actions supplies compilation and the existing Windows test suite.
The suite does not establish Vulkan synchronization correctness or game stability.
No new tests or game/shader-specific hacks were added.

For an initial comparison use the same save, patches, resolution and power mode as
the baseline. Start with 4 GB UMA or Auto, normal PS4 mode, precise readbacks, and
the fork's existing threaded defaults. A game resolution patch determines guest
rendering resolution; changing the emulator window alone does not ensure 720p.
Keep async compute off initially; compare it separately after basic stability.
Do not enable shader dumping, collection, or profiling for FPS measurements.

Check repeated startup, transition into Seattle, traversal and combat for at least
20-30 minutes, memory growth, effects and readbacks. Compare warmed shader-cache
runs. Capture the entire emulator log after an OOM or device loss. Successful
compilation alone does not demonstrate maximum performance or minimal crashes.
