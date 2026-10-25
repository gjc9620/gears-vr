"""Bounded reflected cover observations; never calls game functions or writes memory.

These are bracket-checked reads, not an atomic engine snapshot, a render ticket,
or authorization to change CoverAction. CoverLink/slot flags are reported as
candidates until their relationship to the current controller state is witnessed.
"""
import argparse
import json
import math
from pathlib import Path
import struct
import time

from core import atomic_json
from world_state import Query


class CoverQuery(Query):
    def __init__(self, observer):
        self.values = {}
        super().__init__(observer)

    def remember(self, address, raw):
        key = address, len(raw)
        if key in self.values and self.values[key] != raw:
            raise RuntimeError('Cover value changed during observation')
        self.values[key] = raw
        return raw

    def read(self, address, size):
        # Metadata and bit masks may be visited for multiple selected slots.
        # The inherited Query records latest metadata; keep the first witness
        # too so a mid-sample change cannot silently replace its semantics.
        return self.remember(address, super().read(address, size))

    def value(self, obj, prop):
        return self.remember(obj['address'] + prop['offset'], super().value(obj, prop))

    def finish(self):
        # No append-only relaxation: a rejected sample is useful evidence too.
        super().finish()
        for (address, size), expected in list(self.values.items()):
            if self.read(address, size) != expected:
                raise RuntimeError('Cover value changed at closing boundary')
        super().finish()


def struct_type(q, prop):
    address = prop['address'] + q.r.profile['struct_property_struct']
    raw = q.read(address, 8)
    result = q.r.struct_type(prop)
    if struct.unpack('<Q', raw)[0] != result or q.read(address, 8) != raw:
        raise RuntimeError('Cover structure link changed')
    return result


def boolean(q, obj, name):
    prop = q.prop(obj, name, 'BoolProperty')
    result = q.decode('BoolProperty', q.value(obj, prop), prop['address'])
    if type(result) is not bool:
        raise RuntimeError('Cover Boolean semantics unavailable: ' + name)
    return result


def slot_snapshot(q, link, selected):
    inner, stride, raw, count = q.array(link, 'Slots', 256)
    if q.r.class_name(inner) != 'StructProperty':
        raise RuntimeError('Cover slots are not reflected structures')
    typ = q.obj(struct_type(q, dict(address=inner['address'], type='StructProperty')))
    if typ['name'] != 'CoverSlot':
        raise RuntimeError('Unexpected cover slot structure')
    if not selected or any(type(i) is not int or not 0 <= i < count for i in selected):
        raise RuntimeError('Current cover slot indices escape live array')
    header = q.prop(link, 'Slots', 'ArrayProperty')
    inner_link = q.read(header['address'] + q.r.profile['struct_property_struct'], 8)
    if struct.unpack('<Q', inner_link)[0] != inner['address']:
        raise RuntimeError('Cover slot array inner changed')
    data = struct.unpack('<Qii', q.value(link, header))[0]
    fields = []
    for name, kind in (('bEnabled', 'BoolProperty'), ('bLeanLeft', 'BoolProperty'),
                       ('bLeanRight', 'BoolProperty'), ('bCanPopUp', 'BoolProperty'),
                       ('CoverType', 'ByteProperty'), ('LocationOffset', 'StructProperty'),
                       ('RotationOffset', 'StructProperty')):
        prop = q.prop(typ, name, kind, structure=True)
        if prop['offset'] < 0 or prop['size'] <= 0 or prop['offset'] + prop['size'] > stride:
            raise RuntimeError('Cover slot field escapes its element: ' + name)
        if kind == 'StructProperty':
            nested = q.obj(struct_type(q, prop))
            expected = 'Vector' if name == 'LocationOffset' else 'Rotator'
            if nested['name'] != expected or prop['size'] != 12:
                raise RuntimeError('Cover slot geometry type rejected')
            components = ('X', 'Y', 'Z') if expected == 'Vector' else ('Pitch', 'Yaw', 'Roll')
            component_kind = 'FloatProperty' if expected == 'Vector' else 'IntProperty'
            for offset, component in enumerate(components):
                field = q.prop(nested, component, component_kind, structure=True)
                if field['offset'] != offset * 4 or field['size'] != 4:
                    raise RuntimeError('Cover slot geometry component layout rejected')
        fields.append((name, kind, prop))
    slots = []
    for index in sorted(set(selected)):
        row = raw[index * stride:(index + 1) * stride]
        result = dict(index=index)
        for name, kind, prop in fields:
            start, size = prop['offset'], prop['size']
            value = row[start:start + size]
            q.remember(data + index * stride + start, value)
            if kind == 'StructProperty':
                decoded = list(struct.unpack('<3f' if name == 'LocationOffset' else '<3i', value))
                if not all(math.isfinite(x) for x in decoded):
                    raise RuntimeError('Nonfinite cover slot geometry')
            else:
                decoded = q.decode(kind, value, prop['address'])
                if isinstance(decoded, dict):
                    raise RuntimeError('Unsupported cover slot scalar: ' + name)
            result[name] = decoded
        slots.append(result)
    return dict(count=count, stride=stride, structure=typ, fields=[p for _, _, p in fields], rows=slots)


def capture(observer, *, query_factory=CoverQuery):
    started = time.perf_counter_ns()
    out = dict(schema=1, read_only=True, atomic_snapshot_verified=False,
               renderer_alignment_verified=False, native_cover_state_certified=False,
               started_perf_counter_ns=started, accepted=False)
    try:
        observer.verify()
        if observer.resolver is None or observer.sampler is None:
            raise RuntimeError('Fresh live readers unavailable')
        q = query_factory(observer)
        pc = q.obj(observer.sampler.controller)
        pawn = q.pointer(pc, 'Pawn')
        if not pawn or not q.s.derives(pawn['cls'], 'WarPawn'):
            raise RuntimeError('Cover observation requires the controlled WarPawn')
        if q.pointer(pawn, 'Controller') != pc:
            raise RuntimeError('Cover possession link mismatch')
        level = q.pointer(pawn, 'WorldInfo')
        if not level or q.pointer(pc, 'WorldInfo') != level:
            raise RuntimeError('Cover controller/pawn world mismatch')
        values = {}
        for name, kind, fmt in (('CoverType', 'ByteProperty', '<B'),
                                 ('CoverAction', 'ByteProperty', '<B'),
                                 ('CoverDirection', 'ByteProperty', '<B'),
                                 ('CurrentSlotIdx', 'IntProperty', '<i'),
                                 ('LeftSlotIdx', 'IntProperty', '<i'),
                                 ('RightSlotIdx', 'IntProperty', '<i'),
                                 ('CurrentSlotPct', 'FloatProperty', '<f')):
            values[name] = q.scalar(pawn, name, kind, fmt)
        values['bIsInStationaryCover'] = boolean(q, pawn, 'bIsInStationaryCover')
        link = q.pointer(pawn, 'CurrentLink')
        out.update(controller=pc, pawn=pawn, world=level, link=link, values=values)
        if link:
            if not q.s.derives(link['cls'], 'CoverLink') or q.pointer(link, 'WorldInfo') != level:
                raise RuntimeError('Cover link class/world mismatch')
            values['bCircular'] = boolean(q, link, 'bCircular')
            slots = slot_snapshot(q, link, [values[x] for x in ('CurrentSlotIdx', 'LeftSlotIdx', 'RightSlotIdx')])
        else:
            slots = None
        if not 0 <= values['CurrentSlotPct'] <= 1:
            raise RuntimeError('Cover interpolation fraction outside range')
        q.finish()
        out.update(accepted=True, controller=pc, pawn=pawn, world=level,
                   link=link, values=values, slots=slots,
                   limits='Slot permissions and fields only; not a certified attached state, collision surface, or firing ray.')
    except (RuntimeError, OSError, ValueError, struct.error) as error:
        out.update(rejection=str(error), rejection_type=type(error).__name__)
    out['finished_perf_counter_ns'] = time.perf_counter_ns()
    return out


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--session', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--seconds', type=float, default=10)
    parser.add_argument('--interval', type=float, default=.1)
    args = parser.parse_args()
    if not 0 < args.seconds <= 60 or not .02 <= args.interval <= 1:
        parser.error('Bounded duration/interval required')
    if args.output.exists():
        raise RuntimeError('Preserve existing cover observations')
    from live import Observer
    observer = Observer(args.session)
    rows = []
    try:
        until = time.monotonic() + args.seconds
        while time.monotonic() < until:
            observer.sample()  # Existing process/scene readers own preparation and identity.
            rows.append(capture(observer))
            time.sleep(args.interval)
    finally:
        observer.close()
        args.output.parent.mkdir(parents=True, exist_ok=True)
        atomic_json(args.output, dict(schema=1, read_only=True, records=rows,
                                     accepted=sum(r['accepted'] for r in rows),
                                     native_cover_state_certified=False))


if __name__ == '__main__':
    main()
