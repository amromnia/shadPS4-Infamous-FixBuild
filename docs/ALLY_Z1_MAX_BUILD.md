# Experimental Ally Z1 maximum-performance branch

Branch `perf/ally-z1-max` starts at `3f49a5c0` of `perf/ally-z1-integrated`.
All earlier fixes are inherited. Source research and attribution are in
[ALLY_Z1_MAX_RESEARCH.md](ALLY_Z1_MAX_RESEARCH.md).

## Changes

* Detailed fork performance accounting, timing scopes, CPU stack sampling,
  GPU timestamp queries, fault-history accounting and readback statistics are
  disabled at build time. Tracy is also disabled. Ordinary errors, crash
  assertions and touchpad diagnostics remain. The frame counter required by
  rewrite tracking still advances without a clock query or diagnostic counters.
* Release C/C++ compilation uses O3 and Zen 4 scheduling, plus ThinLTO, linker
  garbage collection and identical-code folding. Instructions remain x86-64-v3
  (AVX2): this is tuning for Zen 4, not an AVX-512-only binary. Build-time tools
  must also run on the Intel build host. Floating-point accuracy is retained.
* Sequential CPU write lookahead grows from at most 16 to at most 64 pages
  (64 to 256 KiB). It marks only non-GPU-dirty pages under the tracker lock;
  mixed windows no longer force the whole optimization to be skipped. Texture
  watchers remain separate. Inspired by MeWeGo `98b92a9289` and the filtered
  write-fault extension in Bloodborne DLSS/FSR PR #16 (`e172285df8`).
* Demand/prefetch readback windows are configurable, testing 1 MiB instead of
  512 KiB, following MeWeGo `3452bc1da7`. Only dirty GPU ranges are copied;
  arena bounds, stale-copy recovery and existing prefetch backoff remain.

## Build options

On Windows, from an initialized MSVC/SDK environment with LLVM, CMake and Ninja
on PATH:

```powershell
cmake --preset ally-z1-max
cmake --build Build/ally-z1-max --target shadps4 --parallel 3
```

`ally-z1-diagnostic` enables the fork profiler again while leaving Tracy off.
Neither preset changes saved game settings or enables Vulkan validation.

Explicit options for other output directories:

| CMake setting | Experimental default | Purpose |
| --- | --- | --- |
| `SHADPS4_ENABLE_PERF_PROFILING` | `OFF` | Compile detailed CPU/GPU performance diagnostics |
| `SHADPS4_Z1_OPTIMIZE` | `ON` | O3, Zen 4 scheduling and Release ThinLTO; Clang required |
| `SHADPS4_READBACK_WINDOW_KIB` | `1024` | Power of two, 4–4096 KiB; try `512` for comparison |
| `SHADPS4_LTO_JOBS` | `3` | Limits compile-time link workers, not gameplay threads |

These are build options, not new launcher switches. Keep a dedicated build
directory for this branch. An optimization comparison with tuning OFF should
also explicitly set `CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE=OFF` when using
an existing cache. The local script uses a bounded ThinLTO cache to speed relinks.

## Local installation

The existing baseline executable remains in `E:\shadps4-local\build\shadPS4.exe`.
The experimental executable is built separately at
`E:\shadps4-local\build-z1\shadPS4.exe`, using `build-z1.cmd` and
`run-build-z1.ps1`. Progress is in `build-z1.log` and `build-z1-status.json`.
The source checkout is on the experimental branch while it builds; the original
`build.cmd` is therefore not a baseline rebuild until the source is switched back
to `perf/ally-z1-integrated`.

## What is still unproven

The memory tests check GPU data preservation, word-boundary handling and
generation/protection updates. They do not establish Vulkan or game correctness.
The 1 MiB window may increase unnecessary copying on shared memory; compare
512 KiB if frame times regress. Async compute and saved runtime settings are
not forced. There is no measured FPS uplift until the user tests on the Ally,
and this change does not claim to fix the earlier device-loss crash.
