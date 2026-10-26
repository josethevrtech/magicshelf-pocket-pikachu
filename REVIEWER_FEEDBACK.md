# Reviewer update: Pocket Pikachu 0.3.5

Hold a controller grip while pointing at the handheld or reaching near it to pick
it up. Release the grip to place it. The existing trigger/housing pickup remains
available. A/B/X/Y and the left D-pad operate the pet while held. The holding
hand's stick adjusts distance; the free hand's stick navigates. A held button
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
