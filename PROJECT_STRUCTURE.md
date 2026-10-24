# Project structure

Apps scaffolded with `mise run new-app` keep the layout of the SDK's LVGL
examples on purpose. `<App>` below is the name passed to `new-app`.

## Layout

```
<project>/
├── mise.toml, mise-tasks/new-app   # tooling, tasks, scaffold script
├── Resources/                      # icon_30x30.png, icon_60x60.png (packed into the .uapp)
├── Output/                         # firmware artifacts: <App>_<ver>.uapp, *.map (gitignored)
└── Software/
    ├── Libs/                       # service code + headers shared with the GUI
    │   ├── Header/                 # see "Service–GUI boundary" below
    │   ├── Sources/                # service sources
    │   └── libs.cmake              # -> LIBS_SOURCES, LIBS_INCLUDE_DIRS
    ├── Apps/
    │   ├── <App>-CMake/            # build entry 1: firmware (CMakeLists.txt only)
    │   └── LVGL-GUI/               # GUI source module ...
    │       ├── gui/{include,src}/  #   screens, widgets, model, theme, GuiApp
    │       ├── assets/             #   fonts/images as generated .c + gen_assets.py
    │       ├── lvgl-gui.cmake      #   -> GUI_APP_SOURCES, GUI_APP_INCLUDE_DIRS
    │       └── simulator/          # ... and build entry 2: host simulator
    └── Output/                     # simulator mock file system (gitignored)
```

| | Firmware | Simulator |
|---|---|---|
| Entry | `Software/Apps/<App>-CMake/CMakeLists.txt` | `Software/Apps/LVGL-GUI/simulator/CMakeLists.txt` |
| Target / compiler | ARM watch, `arm-none-eabi-gcc` | x86-64 Linux, host GCC + SDL2 |
| Service + GUI | two ELFs packed into `Output/<App>_<ver>.uapp` | one executable, `simulator/build/bin/<App>Simulator` |
| Kernel | real UNA kernel | SDK mock kernel |

Both builds compile the same sources: `Software/Libs` and `LVGL-GUI/{gui,assets}`.

## Service–GUI boundary

The service and GUI run as separate processes and talk only through messages.
The SDK examples put every header in one `Libs/Header` directory, and the GUI
build gets all of it on its include path, so GUI code can include service
internals by mistake.

Recommended right after scaffolding:

1. Move the headers the GUI includes (the message and data types, e.g.
   `Commands`, `Settings`, `Track`) to `Libs/Header/common/`, and the rest to
   `Libs/Header/service/`. Check what the GUI includes with
   `grep -rh '#include "' Software/Apps/LVGL-GUI/gui`.
2. In `libs.cmake`, replace `LIBS_INCLUDE_DIRS` with:
   ```cmake
   set(LIBS_COMMON_INCLUDE_DIRS  ${CMAKE_CURRENT_LIST_DIR}/Header/common)
   set(LIBS_SERVICE_INCLUDE_DIRS ${CMAKE_CURRENT_LIST_DIR}/Header/service)
   set(LIBS_INCLUDE_DIRS ${LIBS_COMMON_INCLUDE_DIRS} ${LIBS_SERVICE_INCLUDE_DIRS})
   ```
3. In `<App>-CMake/CMakeLists.txt`, use `${LIBS_COMMON_INCLUDE_DIRS}` in
   `GUI_INCLUDE_DIRS`. The service build and the simulator keep `LIBS_INCLUDE_DIRS`.

No source changes are needed. Including a service header from GUI code then
fails the firmware build. The simulator builds both processes as one target and
does not enforce it.

New code goes where the SDK expects it: service code in `Libs/Header/service`
and `Libs/Sources`, shared message and data types in `Libs/Header/common`, GUI
code in `LVGL-GUI/gui`.

## Why keep the SDK layout

A cleaner layout (separate `firmware/`, `simulator/`, `src/{common,service,gui}`)
was considered and rejected for now:

- Every SDK example, tutorial and doc uses this layout, so copying from them
  stays a direct copy.
- The simulator's file system path is hard-coded relative to the working
  directory (see `KNOWN-ISSUES.md`), which a different layout would have to
  work around.
- SDK updates to the CMake helpers merge without translation.

Reconsider if an app grows into several modules, shares code with another app,
or if the SDK publishes an official blank app template with a different layout.
