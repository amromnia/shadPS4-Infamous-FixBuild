# Ally save-time device loss and touchpad clicks

Investigated `rog_logs/new_crash/shad_log (1).txt` from revision
`d22bdbd0a5589192ac5fe9e2663377ac49e29e54`, running on Ryzen Z1.

The last fatal error is `Device lost during submit` in
`Scheduler::SubmitRecorded`, on the presentation thread. The preceding
`/savedata0/dis.dat` open and close are logged without an error. This does not
prove the save was persisted correctly, or that saving caused the device loss.
There is no explicit out-of-memory error in the supplied log. GPU execution
failure, a driver reset, and invalid GPU work remain possible causes.

Async compute was disabled; threaded command recording and upload threading
were enabled. Submission and presentation use the shared queue lock, so an
unprotected queue submission was not established. No crash workaround is
applied on this evidence alone.

For another reproduction, enable Vulkan validation and synchronization
validation with the validation layer installed. Crash diagnostics additionally
requires `VK_LAYER_LUNARG_crash_diagnostic`; enabling the setting without the
layer does not collect it. Capture the resulting log and any diagnostic dump.
If diagnostics are unavailable, test disabling threaded command recording for
one session, then upload threading separately. These are isolation tests, not
confirmed fixes, and may reduce performance.

Touchscreen automatic click started disabled. The log contains one F1 toggle,
enabling it, and no disable toggle. Automatic click requires two simultaneous
contacts; a single contact is not a mechanical click. F1 does not reset the
controller or GPU.

An independent source-arbitration bug was found in the code: mouse touchpad
polling used the same click flag as the bound controller button, so an idle
mouse could cancel a held manual click. Mouse and manual click state are now
separate. Screen contacts suppress mouse clicks, manual clicks remain usable
during screen gestures, and leaving mouse touchpad mode releases its state.
The supplied log does not establish whether mouse touchpad mode was active
during the interruption. Nine focused regression tests pass.
