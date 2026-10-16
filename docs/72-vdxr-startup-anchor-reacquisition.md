# VDXR startup anchor reacquisition

2026-10-16: actual Quest 3 / VDXR run, game PID27748, session `43c31988e9004f16b4a3dc5d7fc24f89`, Native23 / XR18-vd. Source report: ignored `analysis/runtime/continuous-stereo-27748-20261016-180049-43c31988.json`; native-start log and failure receipts retained alongside the build evidence.

## Observed failure

The level camera was accepted 16.662 seconds after runner start. Native preparation ran its self-tests (4.982 seconds), retried a cross-frame sample, then reacquired after UI registry churn. Its final anchor assembly guard rejected `Object registry changed since resolution`. Native activation did not complete: zero stereo copies and zero projection layers; 6740 startup quad layers remained visible. The runner subsequently requested XR stop, and XR stopped and destroyed its objects. This explains the flat-screen persistence and subsequent VR exit in this run. It does not establish the cause of game stuttering.

The user separately reports foreground game startup avoids the desktop black image; moving the window had also restored the image. Window presentation/redraw remains a hypothesis, not a proven cause.

## Change

The exact strict registry comparison now emits a typed mismatch with the expected and observed valid header. Anchor wrappers preserve this evidence. Before activation, the owner discards that Resolver and all partial anchor/UI certificates and repeats complete acquisition using the existing bounded attempt/deadline policy. It never activates with the rejected certificate. Process/module/player/code/identity failures and untyped matching error text are not admitted as registry churn. The shared outer preparation deadline still applies.

## Validation and remaining work

60 targeted native-capture tests, 8 base anchor tests and 12 cinematic-anchor tests pass. Coverage includes successful fresh reacquisition, exhaustion/deadlines and continued rejection of identity failures. No game/headset rerun is claimed for this change. Next real run must demonstrate native activation plus nonzero stereo copies/projection layers. Startup delay and stuttering remain to measure after this failure is removed.
