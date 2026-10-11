# Touchscreen as a DualShock 4 touchpad

This branch starts at `2d0eb671`, the integrated Ally build before motion commits
`10490c9a`/`58ac0dac` and async-compute correction `571e6595`. It preserves the
earlier OOM, rendering, stability and build fixes. No motion changes are added.

## Use

Touch the focused game window and drag to swipe the emulated touchpad. There is
no activation hotkey, mouse mode, Handheld Companion mapping or extra driver.
The whole game window maps to the pad: top-left is (0,0), bottom-right is
(1919,941). Horizontal, vertical, diagonal and curved movement is reported as
continuous positions; recognizing a gesture remains the game's job. SDL's
window-relative normalized positions also work after resizing and on high-DPI
screens. Black bars, if present, are included in this mapping.

Two fingers are supported, with stable slots and per-contact IDs. Additional
contacts are ignored until a fresh finger-down event. Input goes to player one,
like existing keyboard/mouse emulation. Use the physical controller at the same
time for sticks, buttons and motion.

A screen touch does **not** press the touchpad's mechanical click button. Keep
the existing `touchpad_center`/`touchpad_left`/`touchpad_right` binding (by default
Back/View or keyboard Space for center). For click-and-drag, hold that binding
and drag a finger. The fixed-position binding cannot move a live screen contact.

Canceled touches, minimizing, losing focus and quitting release both contacts.
Gestures starting in captured emulator UI do not reach the game. A second finger
can join a game-owned gesture even if mouse UI capture changes after the first
touch. Already-started
game contacts still receive movement and release if UI capture changes. SDL's
touch-generated mouse events can operate the overlay but are excluded from
guest mouse/key bindings. Mouse and physical touchpad updates yield contact
ownership while any screen finger is down; controller buttons remain active.

## Existing implementations reviewed

- [Upstream mouse-to-touchpad PR #3842](https://github.com/shadps4-emu/shadPS4/pull/3842):
  already in the base; supplies continuous mouse positions, left-button contact
  and right-button click. This implementation reuses its existing guest pad
  state path and makes mouse polling yield safely to screen contacts.
- [Swipe bindings PR #5238](https://github.com/shadps4-emu/shadPS4/pull/5238):
  adds four timed, button-triggered gestures. Reviewed but not imported: native
  finger events provide arbitrary paths without a detached gesture thread.
- [WujekFoliarz touchpad/orientation fork](https://github.com/WujekFoliarz/shadPS4-new-touch-pad-and-orientation):
  older per-contact ID work. The base already assigns IDs when contacts begin,
  so its older pad-read and orientation changes are unnecessary here.
- [Touchpad height PR #5247](https://github.com/shadps4-emu/shadPS4/pull/5247):
  standalone commit `c1a0d439` cherry-picked with attribution; reports height 942.
  The coordinate path additionally keeps x within the reported 1920-pixel width.
- [SDL touch events](https://wiki.libsdl.org/SDL3/SDL_TouchFingerEvent) provide
  device/finger identity, normalized window coordinates and cancellation.

## Validation

The C++ tests cover continuous movement/release, stable two-finger slots,
device identity (including finger ID zero), rejected extra contacts, coordinate
clamping, invalid coordinates on cancellation and stale events after focus loss.
Windows Release build and tests are run manually through GitHub Actions.
Hardware/game testing is still required: automated contact tracking tests do not
prove the ROG Ally driver supplies SDL finger events or that every game accepts
the gestures.

On the Ally, test the Second Son swipe prompt in both directions, then test a
held contact while pressing Back/View. Repeat after lifting and touching again,
with two fingers, and after opening an overlay or switching away from the game.
Touchscreen motion does not implement controller gyro/shake input.

## Two-sided hold followed by an upward swipe

Hold two fingers simultaneously around 20% and 80% of the game window width,
near the lower half of the window, then slide both upward without lifting.
Start away from the physical screen edges (Windows edge gestures can consume
input), and leave room above your fingers for the subsequent upward motion.
Touch contact is distinct from a mechanical touchpad click; use the existing
click binding as well if the prompt specifically requires pressing the pad.

The log now reports `active contacts 2` when both fingers are accepted, along
with down/up positions and rejected initial contacts. If the game still ignores
the gesture, retain that run's log so we can distinguish a missing second OS
contact, misplaced contacts, premature release and a game-side input issue.
Windowless reports route to the focused game window; motion/release for known
contacts remain valid after crossing the window boundary. Disconnecting a
controller also resets contact ownership.
