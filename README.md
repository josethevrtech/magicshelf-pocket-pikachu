# Pocket Pikachu for MagicShelf

A Pocket Pikachu companion inside Wikiti's yellow 3D handheld. The fan
emulator by Alberto-rp runs in Qt's JavaScript engine, with a native LCD and
MagicShelf's stereo renderer. It uses no browser, web server, or network at runtime.

## Play

Turn **Pocket Pikachu** On in MagicShelf. Press the upper round button (**A**) to
open the Poké Ball and meet Pikachu. The lower round button is **B / Back**. The
D-pad selects menus and adjusts values. The two small bottom buttons show
friendship / Select (left) and settings / Start (right). Start opens the original
three options: **RESET**, **SOUND**, and **TIME SET**, initially selecting Sound.
A toggles sound immediately. Time Set changes only this pet's saved clock,
including its daily routines and alarm; the headset clock stays unchanged.
A new pet also offers time setting after hatching.
**B cancels a time edit and returns to settings; B again returns to Pikachu.**
From the clock display, B returns directly to Pikachu. The old fan-emulator
difficulty page is not part of these settings. Back also cancels a pending
A action, so rapid presses cannot reopen a menu after leaving it.

The small recessed circle is the emulator's **full reset**: it clears the pet and
returns to the Poké Ball. **RESET** inside settings clears only today's step
counter. Every physical control has a native beep; hatching and full reset have
short two/three-tone cues. These are synthesized piezo-style sounds, since the
upstream emulator supplies no audio. They play through the headset's default
PipeWire/PulseAudio output. Headset steps themselves are silent.

The LCD stays powered while the creation is On, including after restarting it;
the web emulator's one-minute screen blanking is disabled. There is no simulated
battery to recharge. Pikachu still sleeps at night according to the original
clock and can wake with movement. Saved browser-style dates are parsed explicitly
so a same-day restart cannot incorrectly drop friendship to the Gone state.

Grab the housing or bar beneath it to pick it up. While held, either hand can
operate it: **A** confirms, **B** goes back, **X** shows friendship and **Y** opens
settings. Either stick or the left D-pad navigates the pet while held, including the
holding controller. **Hold the holding stick's click and move that stick up/down**
to push/pull distance; release the click and center the stick before resuming
play. Stick-click no longer confirms. A/B/X/Y remain confirm/back/friendship/settings.
The holding stick takes priority when deflected; otherwise the free stick works.
Dashboard scroll cannot also move distance. **Shake the held controller back and
forth deliberately** to return to the dock, with a haptic confirmation. Menu/View
also returns home. Pickup, release, focus changes and recovery discard held
buttons/directions until fresh input. Headset walking still earns steps; controller
shake recovery does not award steps.
You can also drop it near the shelf.
Controller controls work while pointing at it, using the pointing hand.

On the clock screen, **A toggles the alarm ON/OFF**. With it On, press **Down**
to select the alarm time, then **A** to edit. Left/right selects hour or minute;
up/down changes it; **A saves, B cancels**. Any handheld button dismisses a
ringing alarm; otherwise it stops after 30 seconds. Headset steps do not dismiss
it. With Sound Off it still flashes visually. It fires once per toy-clock day
while the creation is On, including when docked or hidden. Off stops the alarm;
missed alarms are not replayed later.

Headset movement supplies the pedometer: repeated vertical walking/running
motion earns estimated steps while the creation is On, including during other
VR apps and while the handheld is docked or out of view. Sampling requires a
tracked, worn headset. Tracking gaps, position resets, isolated posture changes
and stationary jitter are filtered. This is a head-motion estimate; different
exercise styles may need sensitivity tuning. There is no calorie estimate or
integration with another app's fitness data. Off stops sampling and saves progress.

The clock, watts, gifts, friendship, slot game, daily schedule and animation
unlocks remain. The lifetime counter now uses the original 999,999-step limit,
with the normal progression instead of the emulator's shortened difficulty
modes. Unlocks update during play without needing a restart. Existing steps,
watts, friendship, clock and alarms are preserved across this update. The
visible five-digit counter still resets manually, independently of lifetime steps.
See `FIDELITY.md` for the hardware references and limits of this recreation.

## Source and build

This creation has its own source tree and version. The framework and Digivice
are not modified. The 768 MiB memory cap includes SteamVR's GPU texture-sharing
allocation; a visible overlay used about 360 MiB on the Frame during validation.
Prerequisites: C++17, Python 3, pkg-config, Qt 6 Qml/Widgets,
EGL/GLES3, Cairo, json-c, libpulse-simple and the matching OpenVR library. The tested Frame build
uses its installed Qt 6.8.0. No operating-system packages were installed.

```sh
python3 tools/fetch_sdk.py
python3 tools/fetch_emulator.py
python3 tools/import_model.py '/path/to/pocket_pikachu(1).glb' --output assets
./build.sh
```

Use `OPENVR_LIB_DIR` if OpenVR is outside `/opt/steamvr/bin/linuxarm64` or
`/opt/steamvr/bin/linux64`. The GLB importer targets Wikiti's supplied model;
it removes the baked LCD and preserves the housing, buttons and belt clip.

`./build.sh --tests-only` checks the emulator, alarm, controls and headset motion without the model
or SteamVR. A fresh source checkout needs the two fetch commands first.

For a local desktop preview, run `build/package/bin/pocket-pikachu --desktop`.
Arrow keys control the D-pad, Enter is A, Backspace is B, F shows friendship,
M opens game settings, Shift+R performs a full reset and Space supplies a manual
step. Drag to rotate.
`--preview /absolute/path.png` makes a rendered preview using a temporary save.
The full build generates the creation-page preview this way in `media/`.

## Releases and installation

Build on the target SteamOS version so its native library ABI is respected:

```sh
python3 tools/package_release.py
```

Upload the generated `.magicshelf`, `catalog.json`, `pocket-pikachu.png` and
`SHA256SUMS` together to the matching GitHub release tag, then publish it as the
latest release. Never replace an existing version with different files.

Pocket Pikachu is registered in MagicShelf's official collection. Users can
subscribe and download from Discover; new releases update that subscription.
Uninstalling keeps the Discover listing and saved progress, so it can be
installed again. Public screenshots use the same versioned release as the
package. The source tree excludes imported third-party model files; the built
creation includes the handheld and emulator needed to run it. See
`ASSET_CREDITS.md` for attribution.

`tools/install_local.py` remains available for unpublished development builds.
It verifies the package and preserves other creations and SteamVR. It does not
publish a release or add a creation to Discover for other users.

## Saved data

MagicShelf stores `save.json` and `placement.json` in its private module-data
directory for `org.magicshelf.pocket-pikachu`. Writes are atomic. A corrupt save
is preserved and reported instead of being overwritten. The desktop preview
uses a separate `~/.local/share/magicshelf-pocket-pikachu` profile.

`runtime.json` records current step totals and tracking/sampling diagnostics.
Creating `capture.request` in the private data directory produces `capture.png`
from the current LCD state. Neither diagnostics nor saves are transmitted.

Validation covers all nine physical controls with front/angled hit rays and real
emulator actions, sound cue generation, restart friendship, LCD idle behavior,
menus, saves, held controller routing, daily alarm editing/ringing/persistence, original option controls, clock isolation,
live progression milestones and Frame scroll fallback,
frame-by-frame Back transitions, repeated menu cycles and cancelled A actions,
and headset-motion traces at 30/45/90 Hz. `--sound-test` plays one
short cue without opening or modifying a pet save.
