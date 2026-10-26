"""Focused selected-dependency registry checks for bounded Actor.Trace reads."""
import struct
import unittest

from cover_trace_query import TraceQuery


class Memory:
    def __init__(self):
        self.values = {}
        self.objects_header, self.names_header = 0x100, 0x200
        self.objects_data, self.names_data = 0x1000, 0x2000
        self.object_count, self.object_capacity = 10, 100
        self.name_count, self.name_capacity = 10, 100
        self.name_pointers = [0] * 10
        self.profile = dict(object_internal_index=0, object_name=4,
                            object_class=16, object_outer=24,
                            name_entry_index=0, name_entry_text=20)
        self.put(self.objects_header, struct.pack('<Qii', self.objects_data, 10, 100))
        self.put(self.names_header, struct.pack('<Qii', self.names_data, 10, 100))

    def put(self, address, raw):
        self.values[address, len(raw)] = raw

    def read(self, address, size):
        return self.values[address, size]

    def object(self, *, address=0x5000, index=3, name_index=4, name='CoverLink'):
        entry = 0x6000
        identity = dict(address=address, internal_index=index, name=name,
                        number=1, cls=0x7000, outer=0x8000)
        self.put(address, struct.pack('<I', index))
        self.put(address + 4, struct.pack('<II', name_index, 1))
        self.put(address + 16, struct.pack('<Q', identity['cls']))
        self.put(address + 24, struct.pack('<Q', identity['outer']))
        self.put(self.objects_data + index * 8, struct.pack('<Q', address))
        self.put(self.names_data + name_index * 8, struct.pack('<Q', entry))
        self.put(entry, struct.pack('<I', name_index))
        self.put(entry + 20, name.encode('utf-16-le') + b'\0\0')
        self.name_pointers[name_index] = entry
        return identity


class Observer:
    def __init__(self, resolver, identity):
        self.resolver = resolver
        self.sampler = self
        self.identity = identity

    def fresh(self, address):
        if address != self.identity['address']:
            raise RuntimeError('Unknown selected object')
        return dict(self.identity)

    def verify(self):
        return None


class TraceRegistryTests(unittest.TestCase):
    def fixture(self):
        resolver = Memory()
        identity = resolver.object()
        query = TraceQuery(Observer(resolver, identity))
        self.assertEqual(query.obj(identity['address']), identity)
        return resolver, query

    def test_object_tail_growth_keeps_selected_slot_and_name_fixed(self):
        resolver, query = self.fixture()
        resolver.put(resolver.objects_header,
                     struct.pack('<Qii', resolver.objects_data, 11, 100))
        query.finish()
        self.assertEqual(query.last_object_count, 11)
        self.assertTrue(query.registry_observations)
        self.assertEqual(len(query.slot_witnesses), 1)

    def test_name_growth_and_count_rollback_are_rejected(self):
        resolver, query = self.fixture()
        resolver.put(resolver.names_header,
                     struct.pack('<Qii', resolver.names_data, 11, 100))
        with self.assertRaisesRegex(RuntimeError, 'selected registry growth'):
            query.finish()
        resolver.put(resolver.names_header,
                     struct.pack('<Qii', resolver.names_data, 10, 100))
        resolver.put(resolver.objects_header,
                     struct.pack('<Qii', resolver.objects_data, 11, 100))
        query.finish()
        resolver.put(resolver.objects_header,
                     struct.pack('<Qii', resolver.objects_data, 10, 100))
        with self.assertRaisesRegex(RuntimeError, 'selected registry growth'):
            query.finish()

    def test_observed_growth_cannot_disappear_between_finish_passes(self):
        for header in ('objects', 'names'):
            with self.subTest(header=header):
                resolver, query = self.fixture()
                original_read = resolver.read
                counts = iter((11, 10))
                selected = (resolver.objects_header if header == 'objects'
                            else resolver.names_header)
                data = resolver.objects_data if header == 'objects' else resolver.names_data

                def read(address, size):
                    if address == selected and size == 16:
                        return struct.pack('<Qii', data, next(counts), 100)
                    return original_read(address, size)

                resolver.read = read
                with self.assertRaisesRegex(RuntimeError, 'selected registry growth'):
                    query.finish()

    def test_old_object_or_name_slot_change_is_rejected_despite_growth(self):
        for changed in ('object', 'name'):
            with self.subTest(changed=changed):
                resolver, query = self.fixture()
                resolver.put(resolver.objects_header,
                             struct.pack('<Qii', resolver.objects_data, 11, 100))
                address = (resolver.objects_data + 3 * 8 if changed == 'object'
                           else resolver.names_data + 4 * 8)
                resolver.put(address, struct.pack('<Q', 0x9999))
                with self.assertRaises(RuntimeError):
                    query.finish()

    def test_new_tail_object_cannot_become_trace_dependency(self):
        resolver = Memory()
        identity = resolver.object(index=10)
        query = TraceQuery(Observer(resolver, identity))
        with self.assertRaisesRegex(RuntimeError, 'newly registered tail'):
            query.obj(identity['address'])


if __name__ == '__main__':
    unittest.main()
