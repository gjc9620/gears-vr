# Unified candidate v5

The owner v4 session reached Native/PG and player readiness, then the Python controller raised `Unified equipment connection rejected: 3`. In the hand-participant contract 3 means busy: a PG tick may still be active, or a child attach may have partially connected the parent.

The session now calls the existing exact-cookie stop path and waits for quiescence before reconnecting. Off-state busy stops immediately; partial connections drain before any new start. Retry is bounded to 16 busy attempts; clean exhaustion leaves equipment unavailable without ending first-person/free hands. Drain timeout remains an explicit error. Every attempted start participates in final cleanup, including a failed partial start. Calibration unavailable (1) continues to wait for a real measurement; invalid/ABI/cookie/reentrant errors are not suppressed.

Only `sixdof_unified_runtime.py` changes production behavior. Native/XR/PG are exact v4 artifacts. User Steam path, path history, FOV65 and all config bytes are retained. v4 and its actual failure logs are preserved.

Validation: source/AST and package-file inspection only. No game, XR, simulator, tests, replay or preflight was executed. New owner launch and physical headset acceptance remain pending.
