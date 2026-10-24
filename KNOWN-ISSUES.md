# Known issue: LVGL simulator crashes on 64-bit hosts
AI generated diagnostics

Found with SDK `sdk-v1.5.0`, RunLVGL example, Arch Linux x86-64.

## Symptom

The LVGL simulator crashes after a few seconds, or when opening a list-heavy
screen (e.g. the Intervals menu in RunLVGL). Seen as either:

- `LVGL assert failed ... lv_image.c:150` (`lv_image_set_src: obj != NULL`)
- `SIGSEGV in lv_memset (dst=0x0, ...)`

Afterwards the log repeats `DualAppComm::sendMessage ... Queue is full`, and Esc
or closing the window does nothing, since the GUI thread is gone. Kill it with
`pkill -9 <App>Simulator`.

## Cause

LVGL allocates from a fixed pool, `LV_MEM_SIZE = 40 KB`, set in
`Libs/Header/SDK/Port/LVGL/lv_conf.h`. That size was measured on the watch
(32-bit). The simulator is a 64-bit build, where LVGL objects are larger
because pointers are 8 bytes, so the same screens need about twice the memory.
RunLVGL already uses 50% of the pool on its first screen
(`LVGL pool: 18216/35936 B used`).

When the pool runs out, `lv_image_create()` returns NULL. The SDK's
`SDK::GUI::Draw::image()` (`Libs/Source/GUI/LVGL/Draw.cpp`) passes it on
without a check, which triggers the assert. Other allocations fail the same way
and end in the segfault.

The watch build is not affected.

## Workaround

The simulator uses its own LVGL config, through the SDK's `UNA_LVGL_CONF` hook
(`cmake/una-sdk.cmake`). It includes the SDK config and scales only the pool:

`Software/Apps/LVGL-GUI/simulator/lv_conf_sim.h`:

```c
#include UNA_SDK_LV_CONF
#undef  LV_MEM_SIZE
#define LV_MEM_SIZE (40U * 1024U * sizeof(void*) / 4U)
```

`Software/Apps/LVGL-GUI/simulator/CMakeLists.txt`, before
`include($ENV{UNA_SDK}/cmake/una-simulator.cmake)`:

```cmake
set(UNA_LVGL_CONF "${CMAKE_CURRENT_SOURCE_DIR}/lv_conf_sim.h")
add_compile_definitions("UNA_SDK_LV_CONF=\"$ENV{UNA_SDK}/Libs/Header/SDK/Port/LVGL/lv_conf.h\"")
```

The pool becomes 80 KB on 64-bit and stays 40 KB on 32-bit, so the simulator
keeps the same headroom as the watch. After the change RunLVGL starts at 23%
(`18216/76632 B`) and no longer crashes.

`new-app` does not add this yet; add it by hand to each new app. If the SDK
changes `LV_MEM_SIZE`, update the `40U` here. Remove the workaround once the
SDK fixes the issue.

## Draft GitHub issue

**Title:** LVGL simulator runs out of LVGL memory on 64-bit hosts and crashes

**Body:**

SDK `sdk-v1.5.0`, `Examples/Apps/RunLVGL`, Linux x86-64, GCC.

Steps:

1. Build and run the LVGL simulator as in the RunLVGL tutorial.
2. Wait for the GPS fix (about 5 s), or open the Intervals menu.

Result: `LVGL assert failed ... lv_image.c:150` (`lv_image_set_src: obj != NULL`),
or `SIGSEGV in lv_memset (dst=0x0)`. Then `Queue is full` repeats and the window
cannot be closed.

Cause: `LV_MEM_SIZE` in `Libs/Header/SDK/Port/LVGL/lv_conf.h` is 40 KB, measured
on the 32-bit watch. On a 64-bit simulator build LVGL objects are larger, and
the first screen already uses 50% of the pool (`LVGL pool: 18216/35936 B used`).
When an allocation fails, `SDK::GUI::Draw::image()` in
`Libs/Source/GUI/LVGL/Draw.cpp` passes the NULL from `lv_image_create()` to
`lv_image_set_src()`.

Suggested fix: scale the pool by pointer size in `lv_conf.h`, e.g.
`#define LV_MEM_SIZE (40U * 1024U * sizeof(void*) / 4U)`, or set a larger pool
for simulator builds. Optionally check for NULL in `Draw::image()` and log the
failure.

Workaround: point `UNA_LVGL_CONF` at a simulator-only config that includes the
SDK's `lv_conf.h` and redefines `LV_MEM_SIZE` as above. With 80 KB, RunLVGL
starts at 23% and no longer crashes.

# Known issue: simulator file system path depends on the working directory

The SDK's mock kernel stores files at `../../../../../Output/`, relative to the
directory the simulator runs in (`Libs/Source/Simulator/Kernel/Kernel.cpp`).
Run from `Software/Apps/LVGL-GUI/simulator/build/bin`, as `mise run sim` does,
it resolves to `Software/Output`. Run from anywhere else, files land somewhere
else, possibly outside the project. Keep the simulator layout and run directory
as they are, or the app finds no settings and saves activities elsewhere.
