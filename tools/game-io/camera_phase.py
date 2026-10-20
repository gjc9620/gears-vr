"""Read-only camera-route evidence; no input, hooks or display authorization.

CameraActor is not synonymous with a cinematic. Cinematic mode is decoded only
while the reflected BoolProperty route and verified native code still match.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import struct
import time

from menu_state import MenuReader
from inspect_ui_state import registered
from menu_control import reader_process_identity


class ProcessIdentityChanged(RuntimeError):
    """Do not retry a PID that no longer denotes the selected process."""


class CameraPhaseReader:
    def __init__(self, pid):
        self.reader = MenuReader(pid)
        try:
            self.r = self.reader.r
            self.resolved = self.r.resolve()
            self.camera = int(self.resolved['camera_address'], 16)
            self.controller = int(self.resolved['controller_address'], 16)
            self.local = int(self.resolved['local_player_address'], 16)
            self.creation = reader_process_identity(self.reader)
            self.r.ident_cache.clear()
            self.roles = {name: dict(registered(self.r, address)) for name, address in (
                ('camera', self.camera), ('controller', self.controller), ('local', self.local))}
            camera, pc = self.roles['camera'], self.roles['controller']
            self.fields = {}
            for label, obj, name, kind in (
                ('pawn', pc, 'Pawn', 'ObjectProperty'),
                ('style', camera, 'CameraStyle', 'NameProperty'),
                ('cinematic_storage', pc, 'bCinematicMode', 'BoolProperty'),
                ('view', camera, 'ViewTarget', 'StructProperty')):
                self.fields[label] = dict(self.reader.prop(obj, name, kind))
            view_type = self.r.struct_type(self.fields['view'])
            if registered(self.r, view_type)['name'] != 'TViewTarget':
                raise RuntimeError('Unsupported camera ViewTarget struct')
            self.fields['target'] = self.r.property(view_type, 'Target')
            if self.fields['target']['type'] != 'ObjectProperty':
                raise RuntimeError('Unsupported camera Target type')
            self.field_identity = {name: dict(registered(self.r, p['address'])) for name, p in self.fields.items()}
        except Exception:
            self.close()
            raise

    def close(self):
        self.reader.close()

    def sample(self):
        began = time.perf_counter_ns()
        self.reader.refresh()
        r, original = self.r, self.r.read
        original_name = r.name
        r.names_cache.clear()
        reads = {}
        def observed(address, size):
            raw = original(address, size)
            if (address, size) in reads and reads[address, size] != raw:
                raise RuntimeError('Camera phase changed during sample')
            reads[address, size] = raw
            return raw
        r.read = observed
        def observed_name(index):
            if not 0 <= index < len(r.name_pointers):
                raise RuntimeError('Camera phase name outside registry')
            if r.u64(r.names_data + index * 8) != r.name_pointers[index]:
                raise RuntimeError('Camera phase name registry slot changed')
            return original_name(index)
        r.name = observed_name
        try:
            if reader_process_identity(self.reader) != self.creation:
                raise ProcessIdentityChanged('Camera phase process identity changed')
            for label in ('objects', 'names'):
                header = struct.unpack('<Qii', r.read(getattr(r, label + '_header'), 16))
                pointers = r.object_pointers if label == 'objects' else r.name_pointers
                if header[0] != getattr(r, label + '_data') or header[1] != len(pointers):
                    raise RuntimeError('Camera phase registry moved after refresh: ' + label)
            for role, expected in self.roles.items():
                if registered(r, expected['address']) != expected:
                    raise RuntimeError('Camera phase role changed: ' + role)
                registered(r, expected['cls'])
            for name, prop in self.fields.items():
                if (registered(r, prop['address']) != self.field_identity[name] or
                        r.u32(prop['address'] + r.profile['property_offset_internal']) != prop['offset']):
                    raise RuntimeError('Camera phase property changed: ' + name)
            offsets = self.resolved['offsets']
            for obj, field, target in ((self.controller, 'PlayerCamera', self.camera),
                                      (self.camera, 'PCOwner', self.controller),
                                      (self.controller, 'Player', self.local)):
                if r.u64(obj + int(offsets[field], 16)) != target:
                    raise RuntimeError('Camera phase reciprocal link changed')
            pawn = r.u64(self.controller + self.fields['pawn']['offset'])
            target = r.u64(self.camera + self.fields['view']['offset'] + self.fields['target']['offset'])
            def describe(address):
                if not address:
                    return None
                obj = registered(r, address)
                cls = registered(r, obj['cls'])
                return dict(obj, class_name=cls['name'])
            pawn_object, target_object = describe(pawn), describe(target)
            style, number = struct.unpack('<II', r.read(self.camera + self.fields['style']['offset'], 8))
            style_name = r.name(style)
            storage = r.read(self.controller + self.fields['cinematic_storage']['offset'], 4)
            # Read/copy semantics independently checked in IDA and against live
            # code bytes (docs/65). This is observation, never display authority.
            prop = self.fields['cinematic_storage']['address']
            vtable = r.u64(prop)
            if (r.u64(r.base + 0x1b86280 + 0x2d * 8) != r.base + 0x1d6ea4 or
                    r.u64(vtable + 0x2a0) != r.base + 0x1daba0 or
                    r.u32(prop + 0x70) != 1 or r.u32(prop + 0x74) != 4):
                raise RuntimeError('Unsupported cinematic BoolProperty route/layout')
            for rva, size, digest in (
                    (0x1d6ea4, 127, 'b71e2f794b38111292c497fa82c685f18a50822c44857853bb46acb6ad88609e'),
                    (0x1daba0, 25, 'f4c26fb034bd7529efd10838092da31c9745e7b66f56cd054e793a1957a2915f')):
                if hashlib.sha256(r.read(r.base + rva, size)).hexdigest() != digest:
                    raise RuntimeError('Cinematic BoolProperty native code changed')
            mask = r.u32(prop + 0xd0)
            if not mask or mask & (mask - 1):
                raise RuntimeError('Unsupported cinematic BoolProperty mask')
            cinematic = bool(int.from_bytes(storage, 'little') & mask)
            stamp = struct.unpack('<f', r.read(int(self.resolved['timestamp_address'], 16), 4))[0]
            if not math.isfinite(stamp):
                raise RuntimeError('Invalid camera timestamp')
            for (address, size), raw in reads.items():
                if original(address, size) != raw:
                    raise RuntimeError('Camera phase changed before sample verification')
            ended = time.perf_counter_ns()
            if ended - began > 200000000:
                raise RuntimeError('Camera phase acquisition exceeded 200 ms')
            return dict(pid=r.pid, read_only=True, consistent_reread=True,
                observed_ns=began, finished_ns=ended, camera_timestamp=stamp,
                camera=self.camera, controller=self.controller, pawn=pawn_object,
                view_target=target_object, target_is_pawn=bool(pawn and target == pawn),
                camera_style=[style_name, number], cinematic_storage_raw=storage.hex(),
                cinematic_mask=mask, cinematic_mode=cinematic,
                cinematic_bool_decoded=True, atomic_engine_snapshot=False)
        finally:
            r.read = original
            r.name = original_name


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--pid', type=int, required=True)
    p.add_argument('--creation-time', type=int, required=True)
    p.add_argument('--seconds', type=float, default=30)
    p.add_argument('--out', type=Path, required=True)
    args = p.parse_args()
    if not 0 < args.seconds <= 180:
        p.error('Bounded duration must be at most 180 seconds')
    if args.out.exists():
        raise RuntimeError('Preserve previous camera phase evidence')
    reader = None
    args.out.parent.mkdir(parents=True, exist_ok=True)
    deadline = time.monotonic() + args.seconds
    with args.out.open('x', encoding='utf-8') as stream:
        try:
            while time.monotonic() < deadline:
                try:
                    if reader is None:
                        reader = CameraPhaseReader(args.pid)
                        if reader.creation != (args.pid, args.creation_time):
                            raise ProcessIdentityChanged('Camera phase selected process identity mismatch')
                        stream.write(json.dumps(dict(schema=1, resolved=reader.resolved,
                            fields=reader.fields, roles=reader.roles, creation=reader.creation))+'\n')
                    row = reader.sample()
                except ProcessIdentityChanged as error:
                    stream.write(json.dumps(dict(observed_ns=time.perf_counter_ns(),
                        rejected=str(error), terminal=True))+'\n')
                    stream.flush()
                    raise
                except (RuntimeError, OSError, ValueError) as error:
                    row = dict(observed_ns=time.perf_counter_ns(), rejected=str(error))
                    if reader:
                        reader.close()
                        reader = None
                stream.write(json.dumps(row)+'\n')
                stream.flush()
                time.sleep(.1 if reader else .5)
        finally:
            if reader:
                reader.close()


if __name__ == '__main__':
    main()
