# Mixed native UI scene batch collection

2026-10-17. Offline source work only. This adds the producer collection component needed by the native PathChoice route; it does not enable the root hook or change any rendered UI.

The previous direct text collector selects one current sort key and only accepts simple batches. `ui_scene_batch.h` instead inventories all bounded FCanvas sort elements, recognizing the existing certified simple and material item layouts. It owns copies of meaningful 44-byte simple vertices, 80-byte transform descriptors and material payload evidence. It does not use a subtitle label for native scenes.

Two complete captures must agree on stack/element/item headers, pointer lists and copied contents. A delta matches keys by value, preserves old per-key item order, every old meaningful vertex prefix, and all existing material evidence. New keys, new simple/material items and simple vertex appends are supported. Relocating a vertex allocation is permitted when the owned old prefix remains identical. Removed items/keys, replaced payloads, unknown types, changed old transforms/materials, duplicate identities and failed reads reject without changing the caller's output.

Bounds are 64 keys, 128 total items and 1024 total copied simple vertices, using heap-owned vectors. Simple allocation capacity uses the existing certified 1048576 bound; capacity 2048 with count 4 is valid. Padding after the meaningful 44 bytes is excluded. These limits may reject a larger actual scene and must not be mistaken for proof that it is empty or safe to transform partially.

## Verification

Standalone MSVC `/std:c++17 /W4 /EHsc /UNDEBUG /MT` build and `UiSceneBatchTest` passed. Cases include mixed-key/material appends, key-array reordering, relocated vertex storage, spare capacity, exact limits, malformed caller inventories, prefix edits, item removal/reordering, duplicate keys/items/payloads, unknown types, material changes, read failures and mutations during capture. `inventory` also validates the final owned descriptor/header consistency before publishing it.

The CMake target is `UiSceneBatchTest`. Private build files remain under ignored `analysis/ui-scene-batch-cpu/`. No DLL was built and no game or simulator was launched.

## Integration still required

This is bounded repeated-read consistency, not an engine lock, atomic snapshot, allocator/ABA identity, or proof that a scene did not flush before the root returned. Scene/client/local-player admission, entry/completion checks, an immutable shared group receipt, actual indexed membership/material consumer checks, frame-ticket association and coordinate/cropping compatibility are still required before using the placement helper in production. If a root flushes internally, producer hooks must capture consumed work before that flush; a later delta cannot reconstruct it.

The native route and source provenance are in docs 122 and 124; the affine placement component is in doc 127. Existing producer consumers and subtitle behavior are unchanged.
