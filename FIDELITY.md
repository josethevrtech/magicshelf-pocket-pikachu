# Original yellow Pocket Pikachu fidelity

Target: the monochrome 1998 yellow handheld, not the later Color / 2 GS model.
This runs Alberto-rp's behavioral recreation in a local JavaScript engine; it
is not a ROM or firmware emulator. Version 0.3.0 closes the verified functional
gaps below. It cannot be certified as an exact hardware replica from the
available sources alone.

Version 0.3.1 fixes navigation: leaving the native settings or clock restores
the previous pet frame immediately, without exposing the legacy difficulty
menu. Back/Select/Start cancel pending A transitions, and abandoned animation
timers are cleared. Regression checks cover restored pets during daily routines,
rapid presses, repeated menu cycles, clock/alarm editing and concurrent steps.

## References checked

- [Nintendo's original product description](https://www.nintendo.co.jp/n09/pokepika/page2.html): walking, growing friendship, gifts, activities and the 100-times slot jackpot. The page uses Shift-JIS encoding.
- [Nintendo's archived original-device support](https://web.archive.org/web/20160401024855/http://www.nintendo.com/consumer/systems/other/pokemonpikachu.jsp): the pedometer's 999,999-step limit, screen power saving and real-battery behavior.
- [CountryMouse's firsthand Japanese-device instructions](http://www.bookmice.net/fleur/pikadirect.html), also linked by the upstream emulator: hatch then clock setup; Start menu Reset/Sound/Time; alarm toggling and time selection; 20 steps per watt; five-watt slots and 500-watt jackpot; gifts, friendship and full reset. This is an owner's observation, not Nintendo firmware documentation.
- The pinned upstream source and supplied handheld model, identified in the lock files and asset credits.

## Implemented and checked

| Function | Current behavior / validation |
| --- | --- |
| Holding and placement | Either stick or left D-pad navigates while held. Holding-stick click plus vertical stick adjusts depth; no dashboard-scroll depth duplication. Shared RoomGrab placement and ShakeToDock detector match the other creations. Pickup/release/focus/recovery guards prevent held-input leaks. |
| Physical controls | All nine controls ray-tested from the front and oblique angles; controller A/B/X/Y correspond to A/B/Select/Start. |
| Start options | Original three entries: step counter Reset, Sound, Time Set. Sound is initially selected. |
| Time setting | First-hatch setup plus later editing; saved per-pet clock offset drives clock, daily routines and alarm. OS clock and elapsed-playtime accounting stay independent. |
| Alarm | A toggles On/Off in the clock view; Down then A edits the time. Saved daily alarm; silent visual ringing when sound is disabled; dismissal and restart behavior tested. |
| Step / watt accounting | 20 steps per watt; manual visible-counter reset preserves lifetime steps, watts and friendship. Lifetime limit restored to 999,999. Four-digit watt balance bounded. |
| Progression | Normal-scale thresholds replace the fan emulator's accelerated difficulty modes. Unlock flags and daily routines update without restarting. Existing earned unlocks are retained. |
| Saves | Existing progress and alarm data migrate without resetting. Sound and clock offset survive restart. Atomic saves and corrupt-save preservation remain. |
| Games and pet behavior | Existing slot game, gifts, friendship states, leaving/returning, routines, reactions and animation data remain. Slot payout increments are persisted while animating. |

## Deliberate VR choices and remaining uncertainty

The user explicitly requested continuous power, an always-visible LCD and headset
motion for fitness use. Those choices remain: battery depletion and automatic
screen blanking are disabled. Head movement estimates steps; it cannot reproduce
the mechanical sensor exactly. Turning the creation Off stops its process and
alarm. These are deliberate differences from the physical toy.

Sprite sequences, internal friendship thresholds, lesser slot payouts and odds,
activity timings, synthesized piezo waveforms and the 30-second alarm duration
have not been independently measured against physical hardware or firmware.
The new settings use a native pixel font; their layout is not a traced copy of
the original LCD artwork. No unsupported claim of complete 1:1 accuracy is made.
The original yellow model has no Color/2 GS communication or melody-composer
features; these are not added to this creation.

## 0.3.7 candidate audit (2026-10-26)

Rechecked Nintendo's original yellow-device product page: walking, friendship,
watt gifts, activities/reactions and the 100-times slot jackpot remain the target.
The pinned upstream implementation supplies gifts, slots and routines; existing
native tests cover menu entry, steps/watts, persistence, progression, clock, alarm,
sound and reset. No new hardware-accuracy claim is made. Numeric rules already
based on the owner instructions remain; Nintendo's short product page alone does
not establish their exact values or odds. Color-model infrared and composition
features remain outside this original-model target.

Controller profile paths were checked against the installed SteamVR Frame driver.
Its legacy mapping collapses B/X/Y (and several left D-pad directions), so raw
button guesses cannot provide faithful controls. Named digital actions retain
those distinctions; the attempted legacy joystick fallback was later found unavailable when
actions were dashboard-suppressed and is replaced in 0.3.10. Physical routing still requires the headset.

Dock pose math and SDK are the same as Digivice. The fixed 22 ms sleep plus work
was unsynchronized with SteamVR; the candidate uses WaitFrameSync and records
frame timing in runtime.json. This addresses a pacing risk; perceived moving-
window jitter is not considered resolved until physical comparison. No global
framework, dock configuration, save schema or creation identity is changed.

Frame profiling isolated a repeated LCD stall: the display snapshot used generic
DOM searches for its counter and three icons, each scanning the 1080-pixel tree.
Stable references reduced measured snapshot time from roughly 18-21 ms to
0.7-1.1 ms in the short Frame probe; total paint dropped from roughly 20-27 ms
to 2.5-5.1 ms after warmup. Pixel transport is batched as a string. Runtime
telemetry is atomically replaced without fsync; pet progress keeps durable saves.
These are unattended measurements, not a physical moving-window comparison.

## 0.3.10 dashboard input repair

Physical testing confirmed shake recovery and smooth dock movement in 0.3.7,
but buttons and depth were broken. Live diagnostics showed focused gameplay
with inactive named actions; the legacy GetControllerState call returned no data.
A separate OpenVR probe observed distinct A/B pressed states through
GetComponentStateForDevicePath while those action reads were unavailable.

The Frame-only fallback now reads the driver's named visualization components
for A/B/X/Y, grip, stick click and Menu/View. Stick orientations are matched
against reference poses generated by the installed driver's GetComponentState,
with 0.05 axis spacing. Frame's D-pad uses its documented 7-degree tilt relative
to the same axes as its 20-degree stick. Named actions remain preferred when
available. No global input override setting or other creation is modified.

A live driver-pose check exercised 441 off-grid stick values for each hand:
maximum axis error 0.024 on both, with initialization plus checks under 7 ms.
This verifies pose decoding, not physical end-to-end input. Actual D-pad and
button operation still require user confirmation on the installed candidate.

## 0.3.12 responsiveness candidate

The released 0.3.11 shared the VR tracking thread with emulator timers and saves.
A Frame slot-game probe measured a 211 ms tick, exceeding ShakeToDock's 120 ms
sample-gap limit. Slow ticks both delayed dragging and repeatedly reset gestures.
The runtime had also recorded multi-second loop stalls. The exact attribution of
those longer stalls was not established by the short probe.

VR now owns a separate emulator worker. Only immutable value snapshots and queued
buttons cross threads; each QJSEngine and its QJSValues stay on their owning
thread. Atomic durable saves remain on the worker and shutdown flushes pending
inputs. Tracking, controller following and shake sampling consume the latest LCD
frame without waiting for simulation. Worker timing and dropped-input counts are
included in diagnostics. Desktop/preview modes retain synchronous emulation.

46 string-pixel membership checks now use native includes rather than invoking a
JavaScript predicate per candidate. The same Frame probe dropped from 3183 ms
aggregate / 211 ms worst tick to 259 ms aggregate / 25 ms worst tick. Existing
emulator behavior checks still apply; this changes no friendship or slot rules.
A regression test blocks the worker for 250 ms and checks that tracking-facing
calls return while it remains blocked, then verifies queued steps and persistence.
Physical dragging and shake recovery need headset verification on this candidate.
