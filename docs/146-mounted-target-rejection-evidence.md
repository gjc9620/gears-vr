# Mounted target rejection evidence

2026-10-17. This is a failure-only evidence correction following the unresolved mounted black-screen feedback in [docs120](120-v1-headset-mounted-feedback.md). It does not change rendering or accept a new camera target, and has not been exercised while actually mounted.

Current Python and native mounted certificates both require `Camera.ViewTarget.Target` to equal the controlled Turret. The earlier scout in [docs76](76-mounted-streamed-level-certificates.md) contained no possessed turret and did not establish that relationship during mounted play. There is no demonstrated Python/native serialization mismatch here, and the target assumption remains unverified.

`MountedContextCertificate._capture()` rejected a different target before reaching the ordinary camera guard, so it lost the latter guard's discriminating target evidence. The existing `LiveRegistryAnchorSampler.guard()` already preserves `error.camera_observation`; the mounted early rejection now supplies that bounded field.

The failure receipt retains controlled Turret, Driver, controller and camera identities; target field/value and readable target/class identities; target-is-Driver comparison; timestamp/header expected and observed bytes; bounded mismatches and the first structured read error. `consistent_read` can become true only after fresh checks of all captured identities, pointer/layout/read bytes, registry headers, target and timestamp. Sequential equality is explicitly not an atomic snapshot or admission proof. Diagnostic failures retain the original plain rejection and cannot grant retry authority. The accepted target path retains its existing read schedule and checks.

Tests preserve acceptance of the existing Turret fixture and rejection of Driver/null/foreign targets. They cover changing target/identity/read errors, bounded failure observations and propagation through the outer guard as one non-retryable attempt. The bundled Python passed 126 mounted, anchor, cinematic, integration and world-rebind tests. No native DLL, default, existing ZIP, IDA database or global runtime setting changed.

The next actual mounted observation must distinguish a stable alternative target from role/registry changes and downstream GPU failure. A stable non-Turret target would identify this admission boundary for further investigation; successful schema3 rebind and accepted native anchor tickets would direct investigation downstream. Neither finding is currently established, and neither justifies admitting a Driver/foreign target automatically.
