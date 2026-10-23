# Pocket Pikachu credits

The integration code is MIT licensed. Third-party content keeps its own rights.

- Emulator: [Alberto-rp/Pocket-pikachu-emulator](https://github.com/Alberto-rp/Pocket-pikachu-emulator), pinned in `emulator.lock.json`. The upstream repository describes a non-profit fan project and supplies no source license. The creation bundles the pinned emulator requested by the project owner; the upstream code and artwork retain their original rights.
- Handheld: [Pocket pikachu by Wikiti](https://sketchfab.com/3d-models/pocket-pikachu-f9ea485010804602b96e1ee3f0c5d241), supplied by the user. Embedded metadata identifies the Sketchfab Standard license. Imported meshes and textures stay outside the source repository and are included in the built creation.
- Pokémon and Pikachu characters and marks retain their original owners' rights.
- MagicShelf rendering, placement and controller integration: josethevrtech, MIT.
- OpenVR: Valve, BSD 3-Clause; see `OPENVR-LICENSE.txt`.
- Qt: dynamically linked Qt 6 Core/Qml/Gui/Widgets libraries under their installed licenses. No Qt WebEngine or browser is used.
- Sound cues: original synthesized piezo-style beeps in the integration. These are not recordings from the physical toy; the upstream emulator contains no sound assets. Playback uses the system's PulseAudio-compatible audio service.

The adapter replaces browser DOM/timers/storage with a small local pixel host, removes the optional mobile browser sensor block, adds explicit scopes around three menu switch cases for QJSEngine compatibility, and accelerates the two pixel-writing helpers using sets and direct lookup. It also ignores stale delayed menu actions, parses saved local dates explicitly, rejects invalid or backwards offline-decay intervals, wakes a saved device on launch, and disables the three one-minute LCD idle timers. The local extension adds a saved daily alarm, its pixel editor and synthesized alarm cue, and fixes the clock's noon/midnight labels. It restores Reset/Sound/Time settings, first-hatch clock setup and the 999,999-step progression limit, updates animation unlocks without restarting, and bounds the four-digit watt counter. Device clock changes use a private offset; the operating-system clock is never changed. The original background, game rules and animation data are preserved, including nighttime sleeping.
