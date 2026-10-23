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
| Holding and placement | Holding stick directly pushes/pulls; free stick or D-pad navigates. Same RoomGrab depth implementation as the other creations. Includes Frame dashboard scroll fallback, source-hand checks and prevention of duplicate axis/scroll movement. |
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
