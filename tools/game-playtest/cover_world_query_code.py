"""Read-only code witnesses for the known Actor.Trace wrapper/world route.

The caller supplies its already authenticated current module and CoverQuery,
and MUST close that same query. This observes code, never calls it. Dynamic
dispatch receivers, original query ABI/shape/initial-overlap semantics, current
collision-world synchronization and movement permission remain separate.
"""
import hashlib
from pathlib import Path

import cover_surface_query as surface


# Historical original code. The world collector includes the continuation at
# CFD58..CFEA9 that the earlier IDA procedure record did not retain.
CHUNKS = (
    (0xcdacc, 1873, '44b9510b16dd79df98cd823a89f854b200cf6bad49f6b7c3cd8721c26cff1b22'),
    (0x89b1be, 186, '3d526e6b29f039bbbca1e7df396ec87919bbd4334d499b32cd10a3fe973455f2'),
    (0xcf8f8, 328, '4190a413bace4a59e31431dbf2884463c32fd444279cc3e10f5e2eae27ae6236'),
    (0x89b2ae, 34, 'cb5150c9433a49cdb4563710fe4dabb9f890931ba9fff3e18de627189297a610'),
    (0xcfa40, 1129, '579d89902cd361665d798d433142fd83c9a2369bf4abb7d9b927de236b347a46'),
)


class CodeRejected(RuntimeError):
    def __init__(self, witness):
        super().__init__('World query current code differs: ' + witness['rva'])
        self.source_witness = witness


def _historical(path=surface.DEFAULT_BLOB):
    raw = Path(path).read_bytes()
    if len(raw) != surface.HISTORICAL_SIZE or hashlib.sha256(raw).hexdigest() != surface.HISTORICAL_SHA256:
        raise RuntimeError('World query historical code source differs')
    result = []
    for rva, size, digest in CHUNKS:
        offset = rva - surface.HISTORICAL_RVA
        data = raw[offset:offset + size] if offset >= 0 else b''
        if len(data) != size or hashlib.sha256(data).hexdigest() != digest:
            raise RuntimeError('World query complete original code range differs: ' + hex(rva))
        result.append((rva, data, digest))
    return result


def observe(q, module):
    """Append all five current code witnesses to the caller's read bracket.

    The disk/PE/current-instance certificate is the caller's existing module
    contract. No direct reads bypass q, no engine/virtual calls or new observer
    lifecycle is installed, and no finish result is fabricated here.
    """
    if (module['base'] != q.r.base or module.get('sha256') != surface.GAME_SHA256 or
            q.r.profile['sha256'] != surface.GAME_SHA256):
        raise RuntimeError('World query module/reader identity differs')
    chunks = _historical()
    observed = []
    for rva, expected, digest in chunks:
        surface._range(module, rva, len(expected), True)
        actual = q.read(module['base'] + rva, len(expected))
        actual_digest = hashlib.sha256(actual).hexdigest()
        if actual != expected or actual_digest != digest:
            raise CodeRejected(dict(kind='world_query_code_mismatch', rva=hex(rva), size=len(expected),
                                    expected_sha256=digest, observed_sha256=actual_digest,
                                    observed_size=len(actual), closing_verified=False,
                                    activation_allowed=False))
        observed.append(dict(rva=hex(rva), size=len(expected), sha256=digest))
    return dict(chunks=observed, code_observed=True, closing_required=True,
                engine_calls=False, virtual_calls=False, dynamic_dispatch_verified=False,
                initial_overlap_verified=False, sweep_verified=False,
                world_synchronization_verified=False, activation_allowed=False)
