# Reviewer update: Pocket Pikachu 0.3.5

Hold a controller grip while pointing at the handheld or reaching near it to pick
it up. Release the grip to place it. The existing trigger/housing pickup remains
available. A/B/X/Y and the left D-pad operate the pet while held. Either stick navigates; holding-stick click plus vertical movement adjusts distance. A held button
cannot fire a gameplay action when the creation first gains focus.

When the Steam Bar dock is unavailable at startup, On opens the handheld in room
space after one second of tracked operation. This avoids requiring a separate
Configure action in frametop. Return to shelf also falls back to room placement
if that environment has no usable dock.

The SDK pin includes filtered minification while preserving enlarged pixel-art
edges. The emulator, assets, creation ID, permissions and save locations are
unchanged. Controller tests cover grip press/release, focus gain/loss, held
buttons, two-hand routing and independent depth movement. Emulator, navigation,
alarm, progression, motion and save checks passed; a native source build and
preview also succeeded on the Frame.

Physical controller bindings and the resulting feel still require a headset
test. Frametop hand tracking is not included in this controller update.

Fallback room positions use separate shelf slots rather than stacking creations
at one location. Up to four creations share a row, with additional rows below.


## 0.3.7 development candidate

- Added the shared controller ShakeToDock gesture (previously absent here), with
  head-relative tip sampling, pickup warmup, tracking reset and haptic feedback.
- Either stick now plays while held. Holding-stick click is an explicit depth
  modifier; left D-pad and A/B/X/Y retain their individual actions. Raw joystick
  fallback handles dashboard ownership without guessing collapsed legacy buttons.
- Suppress gameplay on pickup/release, adjustment transitions and shake recovery.
- Use SteamVR frame synchronization instead of a fixed sleep; expose frame timing
  for the moving-window comparison. Perceived jitter requires headset testing.
- Permanent ID, save schema, data paths, subscriptions and other creations stay
  intact. This is an unpublished test build.

### Validation of the final candidate

- Six native test executables passed in WSL and on Frame (engine, alarm,
  navigation, progression, controller and motion). New checks cover display
  snapshot equivalence, shake traces, depth modifier and transition input guards.
- Frame-native 0.3.7 package installed through MagicShelf; On/Off round-trip kept
  progress, sound and alarm. SteamVR PID 2561 and storefront PID 114962 stayed
  unchanged; other creations retained their versions, enabled state and subscriptions.
- LCD profiling before/after is recorded in the framework workspace under
  outputs/pikachu-0.3.7. Snapshot cost fell from about 18-21 ms to around 1 ms.
  The final runtime counted one slow frame during startup and no further >33 ms
  frames over its next 1,248 loops in this unattended sample. Tracking was
  intermittent and the headset was not worn; this is not motion validation.
- Public stable remains 0.3.5. No release, catalog update or Pages site was published.

Physical checks before release: hold in each hand and try both sticks, the left
D-pad and A/B/X/Y; use holding-stick click + vertical movement for depth; release
with a stick deflected and verify no menu input until centered; shake deliberately
and verify dock return without re-pickup; move the Steam window and compare
Pocket Pikachu with Digivice. Check dashboard open and closed.

## 0.3.10 controller follow-up

The user confirmed shake recovery and smooth dock following, but reported failed
controller buttons and distance adjustment. During live A/X/stick testing, the
creation remained focused and held while all named gameplay actions were
inactive. The legacy controller-state fallback was unavailable. Raising overlay
priority did not restore the route and is not retained in the final candidate.

A utility probe observed distinct button presses through the installed driver's
controller visualization components. The focused Frame-only fallback now reads
those components for buttons and matches stick/D-pad orientations against driver
generated reference poses. It does not change global SteamVR input settings.
Trigger-held pickup now watches the trigger component instead of treating every
mouse-button-up event as a release, so face buttons can be used while holding.
Diagnostics expose focus, action errors, fallback use and input/depth counts.

All six native suites passed on Frame. A live driver check passed 441 off-grid
stick samples per hand with maximum error 0.024; initialization and checks took
6.6 ms / 5.3 ms. The final candidate passed On/Off and save-preservation checks;
SteamVR and storefront PIDs and other creation states were unchanged. Physical
end-to-end button/D-pad/depth confirmation is pending. No public release made.

## 0.3.11 direct placement controls

The user confirmed sticks work in 0.3.10, but requested left-hand navigation
and restoration of direct push/pull. The holding stick now adjusts depth without
clicking. With right-hand pickup, the left stick/D-pad navigates. With left-hand
pickup, its stick controls depth and its physical D-pad navigates. Face buttons
retain their mappings. Both placement and navigation require neutral after pickup.
The existing Frame fallback, trigger release fix, shake recovery and smooth dock
following are preserved. Controller checks passed in WSL and on Frame.

The user reported 0.3.11 is much better and authorized publication on 2026-10-26.
The release uses the exact installed package; no rebuild or save migration.

## 0.3.12 unpublished responsiveness repair

The user reported renewed dragging lag and failed shake recovery after release.
A Frame probe reproduced 211 ms slot-game ticks on the tracking thread; the shake
detector rejects gaps above 120 ms. VR emulation now runs on a dedicated worker,
including durable saves. Typed immutable snapshots cross threads and are rebuilt
as QJSValues only in the display engine. Input requests queue without blocking
tracking. Shutdown drains the queue and saves progress; startup failures propagate.

Native pixel-list includes replaced 46 callback membership checks, reducing the
same probe from 3183 ms aggregate / 211 ms worst tick to 259 ms / 25 ms. No game
rules or gesture thresholds changed. Seven native suites passed on Frame; a new
250 ms worker-stall test kept 50 tracking-facing updates responsive (0.023 ms
aggregate), preserved queued steps and verified startup error handling. On/Off
kept progress and other creations; SteamVR PID 2561 and storefront PID 114962
were unchanged. Physical drag/shake testing is pending. No public update published.

The user reported 0.3.12 is much better during the requested headset test.
0.3.13 adds a beginner guide to the creation's About description: startup, Frame
controls, top-icon navigation, steps/watts, gifts, slots, settings/alarm and
placement. The 1,883-character description passes the 2,000-character contract.
Installed 0.3.13 has the identical binary to tested 0.3.12; other creations and
SteamVR/storefront PIDs remained unchanged. This candidate is not published.
