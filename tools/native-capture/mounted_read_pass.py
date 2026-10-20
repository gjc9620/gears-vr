"""One mounted validation pass; bounded selected registry-slot reads only.

Plans contain addresses, never trusted bytes. Each pass owns fresh buffers.
Object headers, the second FName index and name-entry bytes are always read at
their call site. Batched slots move within a pass, not across either boundary;
the independent closing pass remains necessary. This is not an atomic/ABA proof.
"""
import contextlib
import struct

from sample_player_anchor import AnchorRejected


class MountedReadPass:
    MAX_SPAN = 256
    MAX_GAP = 32
    MAX_SLOTS = 256

    def __init__(self, resolver, identities, name_indices):
        self.r = resolver
        self._runs = {}
        self._pending = {}
        # Separate registries even if an unusual layout places their tables on
        # the same page. Bounds use the guard's current registry snapshot.
        for data, count, indices in (
                (resolver.objects_data, resolver.object_count,
                 (value['internal_index'] for value in identities.values())),
                (resolver.names_data, resolver.name_count, name_indices.values())):
            selected = sorted({data + index * 8 for index in indices
                               if type(index) is int and 0 <= index < count})
            if len(selected) > self.MAX_SLOTS:
                raise AnchorRejected('Mounted read plan exceeds selected-slot limit')
            groups = []
            for address in selected:
                if not groups or address // 4096 != groups[-1][0] // 4096 or (
                        address + 8 - groups[-1][0] > self.MAX_SPAN or
                        address - groups[-1][-1] > self.MAX_GAP):
                    groups.append([])
                groups[-1].append(address)
            for group in groups:
                if len(group) > 1:
                    for address in group:
                        self._runs[address] = tuple(group)

    def _read(self, address, size):
        if not address or address < 0 or address > (1 << 64) - size:
            raise AnchorRejected('Invalid mounted identity memory address')
        value = self.r.read(address, size)
        if len(value) != size:
            raise AnchorRejected('Short mounted identity memory read')
        return value

    def _slot(self, address):
        if address in self._pending:
            return struct.unpack('<Q', self._pending.pop(address))[0]
        group = self._runs.get(address)
        if group is None:
            return struct.unpack('<Q', self._read(address, 8))[0]
        start, end = group[0], group[-1] + 8
        block = self._read(start, end - start)
        # Only explicitly selected slots can consume these bytes. A repeated
        # request for a consumed slot forces another live read of its group.
        for member in group:
            self._pending[member] = block[member - start:member - start + 8]
        return struct.unpack('<Q', self._pending.pop(address))[0]

    @contextlib.contextmanager
    def bind(self, sampler):
        if getattr(sampler, '_mounted_read_pass', None) is not None:
            raise AnchorRejected('Nested mounted identity read pass')
        sampler._mounted_read_pass = self
        try:
            yield
        finally:
            del sampler._mounted_read_pass

    def fresh(self, address):
        """Same live identity checks as LiveRegistryAnchorSampler.fresh.

        Resolver.name remains the expected discovery spelling; both the current
        selected name slot and current entry index/text must agree with it.
        """
        r, p = self.r, self.r.profile
        raw = self._read(address, 0x60)
        index = struct.unpack_from('<I', raw, p['object_internal_index'])[0]
        if index >= r.object_count or self._slot(r.objects_data + index * 8) != address:
            raise AnchorRejected('Object registry identity mismatch')
        original_name, number = struct.unpack_from('<II', raw, p['object_name'])
        expected_name = r.name(original_name)
        # Deliberately not sliced from raw or cached with the first header.
        name_index = struct.unpack('<I', self._read(address + p['object_name'], 4))[0]
        if not 0 <= name_index < r.name_count:
            raise AnchorRejected('Live name registry index out of range')
        entry = self._slot(r.names_data + name_index * 8)
        if not entry:
            raise AnchorRejected('Live name entry identity mismatch')
        if (p['name_entry_index'], p['name_entry_text']) == (0, 20):
            # A failed/short mounted batch is terminal. Do not hide a partial
            # read with a later successful retry within the same pass.
            entry_raw = self._read(entry, 148)
            entry_index = struct.unpack_from('<I', entry_raw)[0]
            text = entry_raw[20:148]
        else:
            entry_index = struct.unpack('<I', self._read(entry + p['name_entry_index'], 4))[0]
            text = self._read(entry + p['name_entry_text'], 128)
        if entry_index != name_index:
            raise AnchorRejected('Live name entry identity mismatch')
        end = next((i for i in range(0, 128, 2) if text[i:i + 2] == b'\0\0'), None)
        if end is None:
            raise AnchorRejected('Observer name exceeds 63 UTF16 units')
        name = text[:end].decode('utf-16-le', errors='strict')
        if not name or any(c in name for c in '\r\n\0') or name != expected_name:
            raise AnchorRejected('Name registry identity changed')
        return dict(address=address, internal_index=index, name=expected_name,
                    number=number, cls=struct.unpack_from('<Q', raw, p['object_class'])[0],
                    outer=struct.unpack_from('<Q', raw, p['object_outer'])[0], name_index=name_index)
