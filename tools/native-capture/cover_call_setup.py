"""Prepare an explicitly requested passive cover observer before hook activation.

The sidecars contain current addresses and script bytes. Keep them in the owned
private test instance; they are not release assets or write authorization.
"""
import json
from pathlib import Path


def prepare_ini(pid, profile, dll, expected_camera, *, check_live):
    from resolve_camera import Resolver
    from anchor_config import LiveRegistryAnchorSampler
    from cover_call_config import prepare
    from cover_call_wire import encode

    dll = Path(dll).resolve()
    packet_path = dll.with_name('cover-call-observer.bin')
    proof_path = dll.with_name('cover-call-observer.json')
    if any('\n' in str(p) or '\r' in str(p) for p in (packet_path, proof_path)):
        raise ValueError('Invalid passive cover sidecar path')
    if packet_path.exists() or proof_path.exists():
        raise FileExistsError('Preserve previous passive cover observer evidence')
    check_live()
    resolver = Resolver(pid, profile)
    try:
        camera = resolver.resolve()
        for key in ('module_base', 'local_player_address', 'controller_address', 'camera_address'):
            if camera[key] != expected_camera[key]:
                raise RuntimeError('Cover observer controlled context changed during preparation')
        sampler = LiveRegistryAnchorSampler(resolver, prepared_camera=camera)
        config, provenance = prepare(sampler)
        packet = encode(config)
        check_live()
        packet_path.write_bytes(packet)
        proof_path.write_text(json.dumps({'config': config, 'provenance': provenance},
                                        indent=2, allow_nan=False) + '\n', encoding='utf-8')
    finally:
        resolver.p.close()
    check_live()
    return '\n[CoverCallObserver]\npath=' + str(packet_path) + '\n'
