# Track Run CE

Community edition, not the official UNA Track Run.
The official Track Run app was announced shortly after this was developed, and should probably be preferred over this.

Live pace and distance on a running track, indoors without GPS or outdoors with it.

Pace comes from the selected source: cadence times a stride length from the personal model
the Running app builds outdoors (`SharedData/stride.json`, read only), or GPS outdoors.
Every lap, or distance interval, of known length corrects that source, so the pace follows
the shoes, the fatigue, the bends and the GPS error of the day. The correction is kept per
track (lap length, lane and source) in the app's own `track_calib.json`.

- Mode (Indoor/Outdoor), pace source, lap length, lane and lane width are set in Settings.
  Lane 1 is the inner lane.
- R2 ends a lap. Outdoors, a lap is also counted when GPS passes the start line, which is
  set at Start, or at the first press when the warm-up lap is on.
- In an interval workout R2 ends the phase, and a distance phase is measured against its
  set distance.
- The activity is saved as a running/track FIT file with the correction as developer fields
  (`pace_k`, `lap_anchored`, `lap_trigger`, `distance_model`).

See `idea.md` for the idea and its open questions.

Built on a mise-based dev env closely matching the setup from
UNA SDK docs: https://www.developers.unawatch.com/latest/sdk-setup.html

## Setup

Requires [mise](https://mise.jdx.dev). Linux (Arch or Debian/Ubuntu).

```sh
mise trust
mise bootstrap   # OS packages, una-sdk clone, tools, LVGL submodule + Python venv
mise doctor      # optional health check
```

`UNA_SDK`, the ARM toolchain and the venv are automatically set inside this directory.

### Individual steps

```sh
mise bootstrap packages status   # which OS packages are missing
mise bootstrap packages apply    # install only the OS packages (asks for sudo)
mise bootstrap repos apply       # clone una-sdk at the pinned tag
mise install                     # ARM toolchain, cmake, ninja, node, uv
mise run bootstrap               # LVGL submodule + uv sync
```

`mise bootstrap` also applies any `[bootstrap]` sections in your global mise config.
To apply only this project's parts use: `mise bootstrap --only packages,repos,tools,task`.
Package declarations always merge with the global config.
To install strictly this project's packages, hide the global config:
`MISE_GLOBAL_CONFIG_FILE=$(mktemp --suffix .toml) mise bootstrap packages apply`.

## Build, test and simulate

```sh
mise run build   # .uapp -> Output/
mise run test    # host unit tests (needs googletest installed)
mise run sim     # build + run the Linux simulator (keys 1-4 = L1 L2 R1 R2, 5 = wrist, Esc = quit)
mise run assets  # regenerate LVGL fonts/images
mise run icons   # regenerate the app icons in Resources/
mise run screenshots  # headless simulator tours -> screenshots/
mise run clean
```

`mise run sim --config <file> --fs <dir>`: `--config` copies a companion app config to
`app_config.json`; `--fs` copies a folder's contents. Both go into the simulator's file
system (`Software/Output`) and stay there.

## Releasing

Needs `Output/app-manifest.json` ([docs](https://www.developers.unawatch.com/latest/app-config-json.html))
with `appVersion`, `"binary": "<App>_<version>.uapp"` and `"icon": "icon.png"`.

```sh
git tag apps-v0.1.0     # must match appVersion
mise run package        # -> dist/<App>_0.1.0.zip
```

Upload the zip on the app's **Version** tab at https://apps.unawatch.com and release it
([docs](https://www.developers.unawatch.com/latest/deploy.html#sharing-the-apps)).

- Firmware build: https://www.developers.unawatch.com/latest/sdk-setup.html
- Linux simulator: https://www.developers.unawatch.com/latest/Tutorials/RunLVGL/ARCHITECTURE.html
