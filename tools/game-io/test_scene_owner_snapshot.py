"""Synthetic raw reflection/registry fixtures; no OS process handles or input."""
import copy
import json
from pathlib import Path
import struct
import unittest
from unittest import mock

from scene_owner_snapshot import capture_scene_owner, SceneOwnerRejected, GAME_SHA, _reread_spans, _Capture, read_conflict_error


class Fixture:
    def __init__(self, null_owner=False):
        self.base, self.pid = 0x140000000, 123
        self.profile = json.loads((Path(__file__).resolve().parents[1] / 'package-inspector/reloaded-profile.json').read_text())
        self.memory, self.objects, self.names, self.classes = {}, [], ['None'], {}
        self.objects_data, self.names_data = 0x100000, 0x200000
        self.next_address = 0x400000
        self.field_lists, self.props = {}, {}
        self.ident_cache, self.names_cache = {'poison': 'bad'}, {'poison': 'bad'}
        self.identity = self.name = mock.Mock(side_effect=AssertionError('Resolver cache method used'))
        self.property = mock.Mock(side_effect=self.property_hint)
        self.hint_overrides = {}
        self.hook, self.read_counts = None, {}
        class_address = self.allocate()
        self.classes['Class'] = class_address
        self.obj('Class', class_address, address=class_address)
        self.put(class_address + 96, '<Q', 0)
        self.put(class_address + 128, '<Q', 0)
        for name, parent in (('Object', None), ('ObjectProperty', 'Object'), ('ArrayProperty', 'Object'),
                             ('WrongProperty', 'Object'), ('Engine', 'Object'), ('Player', 'Object'),
                             ('LocalPlayer', 'Player'), ('PlayerController', 'Object'), ('WarPC', 'PlayerController'),
                             ('Interaction', 'Object'), ('UIInteraction', 'Interaction'), ('WarUIInteraction', 'UIInteraction'),
                             ('GameViewportClient', 'Object'), ('GameUISceneClient', 'Object'),
                             ('UIScene', 'Object'), ('UIScene_PathChoice', 'UIScene')):
            address = self.obj(name, class_address)
            self.classes[name] = address
            self.put(address + 96, '<Q', self.classes[parent] if parent else 0)
            self.put(address + 128, '<Q', 0)
        self.engine = self.obj('Engine_0', self.classes['Engine'])
        self.viewport = self.obj('Viewport_0', self.classes['GameViewportClient'], self.engine)
        self.interaction = self.obj('Interaction_0', self.classes['WarUIInteraction'], self.viewport)
        self.client = self.obj('GameUISceneClient', self.classes['GameUISceneClient'], self.interaction)
        self.local = self.obj('Local_0', self.classes['LocalPlayer'], self.engine)
        self.pc = self.obj('PC_0', self.classes['WarPC'])
        self.scene = self.obj('DeltaChoosePath', self.classes['UIScene_PathChoice'])
        for owner, name, kind, offset in (('UIInteraction', 'SceneClient', 'ObjectProperty', 0x200),
                ('GameViewportClient', 'UIController', 'ObjectProperty', 0x208),
                ('LocalPlayer', 'ViewportClient', 'ObjectProperty', 0x210), ('Player', 'Actor', 'ObjectProperty', 0x218),
                ('PlayerController', 'Player', 'ObjectProperty', 0x220), ('Engine', 'GamePlayers', 'ArrayProperty', 0x228),
                ('GameUISceneClient', 'ActiveScenes', 'ArrayProperty', 0x238),
                ('UIScene', 'SceneClient', 'ObjectProperty', 0x248), ('UIScene', 'PlayerOwner', 'ObjectProperty', 0x250)):
            self.prop(owner, name, kind, offset)
        for address, offset, value in ((self.interaction, 0x200, self.client), (self.viewport, 0x208, self.interaction),
                (self.local, 0x210, self.viewport), (self.local, 0x218, self.pc), (self.pc, 0x220, self.local),
                (self.scene, 0x248, self.client), (self.scene, 0x250, 0 if null_owner else self.local)):
            self.put(address + offset, '<Q', value)
        self.array(self.engine + 0x228, [self.local])
        self.array(self.client + 0x238, [self.scene])
        self.sync()

    def allocate(self):
        result = self.next_address
        self.next_address += 0x1000
        return result

    def put(self, address, fmt, *values):
        raw = struct.pack(fmt, *values)
        for offset, value in enumerate(raw):self.memory[address + offset] = value

    def bytes(self, address, raw):
        for offset, value in enumerate(raw):self.memory[address + offset] = value

    def obj(self, name, cls, outer=0, address=None):
        address = address or self.allocate()
        index = len(self.objects)
        self.objects.append(address)
        if name not in self.names:self.names.append(name)
        self.put(address + 8, '<I', index)
        self.put(address + 72, '<II', self.names.index(name), 0)
        self.put(address + 80, '<Q', cls)
        self.put(address + 64, '<Q', outer)
        return address

    def prop(self, owner_name, name, kind, offset):
        owner = self.classes[owner_name]
        address = self.obj(name, self.classes[kind], owner)
        self.put(address + 112, '<II', 1, 16 if kind == 'ArrayProperty' else 8)
        self.put(address + 148, '<I', offset)
        fields = self.field_lists.setdefault(owner, [])
        if fields:self.put(fields[-1] + 104, '<Q', address)
        else:self.put(owner + 128, '<Q', address)
        self.put(address + 104, '<Q', 0)
        fields.append(address)
        self.props[owner_name, name] = address
        if kind == 'ArrayProperty':
            inner = self.obj('Inner_' + name, self.classes['ObjectProperty'], address)
            self.put(inner + 112, '<II', 1, 8)
            self.put(address + 208, '<Q', inner)
        return address

    def array(self, address, values):
        data = self.allocate() if values else 0
        self.put(address, '<Qii', data, len(values), len(values))
        for index, value in enumerate(values):self.put(data + index * 8, '<Q', value)
        return data

    def sync(self):
        self.put(self.base + 0x1c24730, '<Qii', self.objects_data, len(self.objects), len(self.objects))
        self.put(self.base + 0x1c34800, '<Qii', self.names_data, len(self.names), len(self.names))
        for index, address in enumerate(self.objects):self.put(self.objects_data + index * 8, '<Q', address)
        for index, name in enumerate(self.names):
            entry = 0x300000 + index * 0x100
            self.put(self.names_data + index * 8, '<Q', entry)
            self.put(entry, '<I', index)
            self.bytes(entry + 20, (name + '\0').encode('utf-16-le'))

    def read(self, address, size):
        key = address, size
        self.read_counts[key] = self.read_counts.get(key, 0) + 1
        if self.hook:self.hook(address, size)
        try:return bytes(self.memory[address + offset] for offset in range(size))
        except KeyError as error:raise SceneOwnerRejected('Synthetic unreadable source') from error

    def mutate_on_reread(self, target):
        # A final verification span may cover several adjacent original reads.
        # Mutate the same underlying byte regardless of the read's boundaries.
        hits = 0
        def mutate(address, size):
            nonlocal hits
            if address <= target < address + size:
                hits += 1
                if hits >= 2:self.memory[target] ^= 1
        return mutate

    def property_hint(self, cls, name, inherited=False):
        if (cls, name) in self.hint_overrides:
            return dict(address=self.hint_overrides[cls, name], offset='poisoned', type='poisoned')
        current = cls
        for _ in range(64):
            owner = next(key for key, address in self.classes.items() if address == current)
            if (owner, name) in self.props:
                return dict(address=self.props[owner, name], offset='poisoned', type='poisoned')
            current = struct.unpack('<Q', bytes(self.memory[current + 96 + i] for i in range(8)))[0]
            if not inherited or not current:break
        raise RuntimeError('No fixture discovery hint')

    def capture(self):
        return capture_scene_owner(self, self.client, self.scene, self.pc, self.local)


class SceneOwnerTests(unittest.TestCase):
    def test_conflict_reports_exact_headers_without_misclassifying_other_spans(self):
        expected=struct.pack('<Qii',0x9000,1001,2000)
        observed=struct.pack('<Qii',0x9000,1013,2000)
        for address,size,registry in [(0x100,16,True),(0x300,16,False),(0x100,8,False)]:
            error=read_conflict_error(SceneOwnerRejected,'changed','owner_repeated_read',
                address,expected[:size],observed[:size],{'objects':0x100})
            detail=error.read_conflict
            self.assertEqual(detail['stage'],'owner_repeated_read')
            self.assertEqual(detail['address'],hex(address))
            self.assertEqual('registry' in detail,registry)
            if registry:
                self.assertEqual(detail['initial_raw'],expected.hex())
                self.assertEqual(detail['observed_raw'],observed.hex())
                self.assertEqual(detail['initial'],dict(data='0x9000',num=1001,max=2000))
                self.assertEqual(detail['observed']['num'],1013)

    def test_owner_final_registry_bracket_reports_header_without_extra_read(self):
        c=_Capture.__new__(_Capture)
        c.identities={};c.base=1;c.pid=2;c.r=mock.Mock(base=1,pid=2)
        c.headers={'objects':0x100,'names':0x200}
        expected=struct.pack('<Qii',0x9000,1001,2000)
        observed=struct.pack('<Qii',0x9000,1013,2000)
        c.reads={(0x100,16):expected}
        c._read=mock.Mock(side_effect=[expected,observed])
        with self.assertRaises(SceneOwnerRejected) as raised:c.finish()
        detail=raised.exception.read_conflict
        self.assertEqual(detail['stage'],'owner_registry_bracket')
        self.assertEqual(detail['initial']['num'],1001)
        self.assertEqual(detail['observed']['num'],1013)
        self.assertEqual(c._read.call_args_list,[mock.call(0x100,16),mock.call(0x100,16)])

    def test_owner_final_conflict_uses_same_read_and_only_changed_window(self):
        c=_Capture.__new__(_Capture)
        c.identities={};c.base=1;c.pid=2
        c.r=mock.Mock(base=1,pid=2)
        c.headers={'objects':0x100,'names':0x200}
        c.reads={(0x300,16):b'a'*16}
        c._read=mock.Mock(return_value=b'a'*5+b'Z'+b'a'*10)
        with self.assertRaises(SceneOwnerRejected) as raised:c.finish()
        detail=raised.exception.read_conflict
        self.assertEqual(detail['stage'],'owner_final_reread')
        self.assertEqual(detail['changed_address'],'0x305')
        self.assertEqual(detail['expected_window'],(b'a'*8).hex())
        self.assertEqual(detail['actual_window'],(b'Z'+b'a'*7).hex())
        self.assertNotIn('registry',detail)
        c._read.assert_called_once_with(0x300,16)

    def test_exported_name_entries_are_from_this_capture(self):
        f=Fixture();result=f.capture();entries={x['index']:x for x in result['name_entries']}
        self.assertEqual(entries[0]['text'],'None')
        for identity in result['identities']:
            entry=entries[identity['name_index']]
            self.assertEqual(entry['text'],identity['name'])
            pointer=struct.unpack('<Q',f.read(f.names_data+identity['name_index']*8,8))[0]
            self.assertEqual(entry['address'],pointer)
            self.assertEqual(f.read(pointer+20,(len(entry['text'])+1)*2),(entry['text']+'\0').encode('utf-16-le'))

    def test_array_allocator_slack_is_distinct_from_member_limit(self):
        f=Fixture()
        f.put(f.engine+0x228+12,'<i',17)
        f.put(f.client+0x238+12,'<i',257)
        result=f.capture()
        self.assertEqual(result['owner_classification'],'local_player')
        for count,capacity in ((17,17),(1,4097),(2,1),(-1,17),(1,-1)):
            with self.subTest(count=count,capacity=capacity):
                f=Fixture();f.put(f.engine+0x228+8,'<ii',count,capacity)
                with self.assertRaisesRegex(SceneOwnerRejected,'Invalid owner array'):f.capture()

    def test_unrelated_numbered_fields_preserve_exact_property_identity(self):
        for name in ('CONTEXT_ROUNDSCORE_ANNEX','Player'):
            with self.subTest(name=name):
                f=Fixture();field=f.prop('WarPC',name,'ObjectProperty',0x300)
                f.put(field+76,'<I',481)
                f.props.pop(('WarPC',name))
                f.hint_overrides[f.classes['WarPC'],'Player']=f.props['PlayerController','Player']
                f.sync();result=f.capture()
                self.assertEqual(result['owner_classification'],'local_player')
                # Numbered fields remain owned, registered and re-read evidence.
                bad_outer=f.classes['PlayerController'];f.put(field+64,'<Q',bad_outer)
                with self.assertRaisesRegex(SceneOwnerRejected,'outer/name'):f.capture()

    def test_required_numbered_property_is_not_accepted_as_unnumbered(self):
        f=Fixture();f.put(f.props['PlayerController','Player']+76,'<I',481)
        with self.assertRaisesRegex(SceneOwnerRejected,'hint name mismatch'):f.capture()

    def test_unrelated_number_change_during_capture_rejects(self):
        f=Fixture();field=f.prop('WarPC','CONTEXT_ROUNDSCORE_ANNEX','ObjectProperty',0x300)
        f.put(field+76,'<I',481);f.sync()
        f.hook=f.mutate_on_reread(field+76)
        with self.assertRaises(SceneOwnerRejected):f.capture()

    def test_resolver_read_failures_are_explicit_rejection(self):
        for error in (OSError('unreadable'), RuntimeError('process changed')):
            with self.subTest(error=type(error).__name__):
                f = Fixture()
                f.read = mock.Mock(side_effect=error)
                with self.assertRaisesRegex(SceneOwnerRejected, 'source read failed'):
                    f.capture()

    def test_fresh_graph_and_owner_classifications_ignore_resolver_caches(self):
        for null_owner in (False, True):
            with self.subTest(null_owner=null_owner):
                f = Fixture(null_owner)
                result = f.capture()
                self.assertEqual(result['owner_classification'], 'unowned_single_local_player' if null_owner else 'local_player')
                self.assertEqual(result['roles']['interaction']['name'], 'Interaction_0')
                self.assertEqual(result['profile_game_sha256'], GAME_SHA)
                self.assertTrue(result['consistent_reread'])
                for key in ('atomic_snapshot', 'allocation_identity_proven', 'render_admission', 'gpu_verified'):
                    self.assertFalse(result[key])
                self.assertGreater(len(result['properties']), 8)
                f.identity.assert_not_called();f.name.assert_not_called();self.assertTrue(f.property.called)
                self.assertNotIn((f.client, 96), f.read_counts)
                self.assertNotIn((f.client, 1), f.read_counts)
                self.assertLess(result['evidence']['total_read_calls'], result['evidence']['max_read_calls'])
                json.dumps(result)

    def test_large_real_scale_field_chains_use_lightweight_fresh_headers(self):
        f = Fixture()
        # Saved schema has 1359 immediate WarPC fields and 453 PlayerController
        # fields. Give unrelated fields distinct names; none need text decoding.
        irrelevant = []
        for owner, count in (('WarPC', 1359), ('PlayerController', 452)):
            for index in range(count):
                irrelevant.append(f.prop(owner, 'Unused_' + owner + '_' + str(index), 'ObjectProperty', 0x300))
        f.sync()
        result = f.capture()
        self.assertLess(result['evidence']['total_read_calls'], 32768)
        self.assertEqual(max(len(item['headers']) for item in result['fields']), 1359)
        for address in irrelevant:
            self.assertNotIn((address + 80, 8), f.read_counts)
            name_index = struct.unpack('<I', bytes(f.memory[address + 72 + i] for i in range(4)))[0]
            self.assertNotIn((f.names_data + name_index * 8, 8), f.read_counts)
        # Reproduce a cold discovery pass using the same fixed global budget.
        # The old one-read-per-ledger-entry closure exceeds 32768 with this
        # work; merged final spans retain all evidence and leave room for it.
        cold = True
        def hint(cls, name, inherited):
            nonlocal cold
            if cold:
                cold = False
                for _ in range(14000):f.read(f.base + 0x1c24730,16)
            return f.property_hint(cls,name,inherited)
        f.property.side_effect = hint
        with_discovery = f.capture()
        self.assertEqual(with_discovery['evidence']['discovery_hint_read_calls'],14000)
        self.assertLess(with_discovery['evidence']['total_read_calls'],32768)
        self.assertEqual(with_discovery['evidence']['source_bytes_sha256'],result['evidence']['source_bytes_sha256'])
        # A late mutation in an unrelated header is still rejected.
        target = irrelevant[10] + 104
        f.read_counts.clear()
        f.hook = f.mutate_on_reread(target)
        with self.assertRaises(SceneOwnerRejected):f.capture()

    def test_hint_reads_share_budget_and_nested_reader_is_restored(self):
        f = Fixture();original = f.read
        def hint(cls, name, inherited):
            f.read(f.base + 0x1c24730, 16)
            return f.property_hint(cls, name, inherited)
        f.property.side_effect = hint
        result = f.capture()
        self.assertEqual(result['evidence']['discovery_hint_read_calls'], 9)
        self.assertEqual(f.read, original)
        with mock.patch('scene_owner_snapshot.MAX_READS', 100):
            with self.assertRaises(SceneOwnerRejected):f.capture()
        self.assertEqual(f.read, original)

    def test_discovery_hint_wrong_unreachable_and_shadowed_rejected(self):
        for kind in ('wrong', 'unreachable', 'shadowed', 'duplicate'):
            with self.subTest(kind=kind):
                f = Fixture()
                selected = f.props['PlayerController', 'Player']
                if kind == 'wrong':
                    f.hint_overrides[f.classes['WarPC'], 'Player'] = f.props['UIScene', 'PlayerOwner']
                elif kind == 'unreachable':
                    detached = f.obj('Player', f.classes['ObjectProperty'], f.classes['PlayerController'])
                    f.put(detached + 112, '<II', 1, 8)
                    f.put(detached + 148, '<I', 0x220)
                    f.hint_overrides[f.classes['WarPC'], 'Player'] = detached
                    f.sync()
                elif kind == 'shadowed':
                    f.prop('WarPC', 'Player', 'ObjectProperty', 0x260)
                    f.put(f.pc + 0x260, '<Q', f.local)
                    f.hint_overrides[f.classes['WarPC'], 'Player'] = selected
                    f.sync()
                else:
                    f.prop('PlayerController', 'Player', 'ObjectProperty', 0x260)
                    f.hint_overrides[f.classes['WarPC'], 'Player'] = selected
                    f.sync()
                with self.assertRaises(SceneOwnerRejected):f.capture()

    def test_property_type_dimension_size_outer_inner_and_offset_rejected(self):
        for failure in ('type', 'dimension', 'size', 'outer', 'offset', 'inner_type', 'inner_outer', 'inner_size', 'inner_dimension'):
            with self.subTest(failure=failure):
                f = Fixture();prop = f.props['GameUISceneClient', 'ActiveScenes']
                inner = struct.unpack('<Q', f.read(prop + 208, 8))[0]
                if failure == 'type':f.put(prop + 80, '<Q', f.classes['ObjectProperty'])
                elif failure == 'dimension':f.put(prop + 112, '<I', 2)
                elif failure == 'size':f.put(prop + 116, '<I', 8)
                elif failure == 'outer':f.put(prop + 64, '<Q', f.classes['UIScene'])
                elif failure == 'offset':f.put(prop + 148, '<I', 0xffff)
                elif failure == 'inner_type':f.put(inner + 80, '<Q', f.classes['ArrayProperty'])
                elif failure == 'inner_outer':f.put(inner + 64, '<Q', f.classes['Engine'])
                elif failure == 'inner_size':f.put(inner + 116, '<I', 16)
                else:f.put(inner + 112, '<I', 2)
                with self.assertRaises(SceneOwnerRejected):f.capture()

    def test_graph_identity_class_slots_and_player_scope_rejected(self):
        for failure in ('client_class', 'scene_class', 'class_cycle', 'default', 'slot', 'name_slot', 'name_index',
                        'local_outer', 'backlink', 'pc_link', 'viewport_link', 'duplicate_scene', 'missing_scene',
                        'other_owner', 'multiple_players', 'wrong_player'):
            with self.subTest(failure=failure):
                f = Fixture(True)
                if failure == 'client_class':f.put(f.client + 80, '<Q', f.classes['UIScene'])
                elif failure == 'scene_class':f.put(f.scene + 80, '<Q', f.classes['UIScene'])
                elif failure == 'class_cycle':f.put(f.classes['UIScene'] + 96, '<Q', f.classes['UIScene_PathChoice'])
                elif failure == 'default':
                    index=f.names.index('DeltaChoosePath');f.bytes(0x300000+index*0x100+20, 'Default__X\0'.encode('utf-16-le'))
                elif failure == 'slot':f.put(f.objects_data + f.objects.index(f.client) * 8, '<Q', f.scene)
                elif failure == 'name_slot':f.put(f.names_data + f.names.index('DeltaChoosePath') * 8, '<Q', 0)
                elif failure == 'name_index':f.put(f.client + 72, '<I', 0xffffffff)
                elif failure == 'local_outer':f.put(f.local + 64, '<Q', f.viewport)
                elif failure == 'backlink':f.put(f.interaction + 0x200, '<Q', 0)
                elif failure == 'pc_link':f.put(f.pc + 0x220, '<Q', f.scene)
                elif failure == 'viewport_link':f.put(f.viewport + 0x208, '<Q', f.client)
                elif failure == 'duplicate_scene':f.array(f.client + 0x238, [f.scene, f.scene])
                elif failure == 'missing_scene':f.array(f.client + 0x238, [])
                elif failure == 'other_owner':f.put(f.scene + 0x250, '<Q', f.pc)
                elif failure == 'multiple_players':f.array(f.engine + 0x228, [f.local, f.pc])
                else:f.array(f.engine + 0x228, [f.pc])
                with self.assertRaises(SceneOwnerRejected):f.capture()

    def test_midread_identity_layout_link_registry_and_name_mutations_reject(self):
        for kind in ('identity', 'super', 'field_next', 'property', 'inner', 'link', 'array', 'objects', 'names', 'text', 'kind_super', 'slot'):
            with self.subTest(kind=kind):
                f = Fixture();prop = f.props['UIScene', 'PlayerOwner']
                targets = dict(identity=(f.client+64,8), super=(f.classes['UIScene']+96,8),
                               field_next=(prop+104,8), property=(prop+148,4),
                               inner=(f.props['Engine','GamePlayers']+208,8), link=(f.pc+0x220,8),
                               array=(f.client+0x238,16), objects=(f.base+0x1c24730,16),
                               names=(f.base+0x1c34800,16),
                               kind_super=(f.classes['ObjectProperty']+96,8),
                               slot=(f.objects_data+f.objects.index(f.scene)*8,8), text=(0x300000+f.names.index('DeltaChoosePath')*0x100+20,2))
                target = targets[kind]
                f.hook = f.mutate_on_reread(target[0])
                with self.assertRaises(SceneOwnerRejected):f.capture()

    def test_final_spans_cover_only_observed_bytes_and_detect_overlap_conflicts(self):
        reads={(10,2):b'ab',(12,2):b'cd',(11,2):b'bc',(20,2):b'ef'}
        self.assertEqual(list(_reread_spans(reads)),[(10,b'abcd'),(20,b'ef')])
        with self.assertRaisesRegex(SceneOwnerRejected,'overlapping'):
            list(_reread_spans({(10,4):b'abcd',(11,2):b'XX'}))
        with mock.patch('scene_owner_snapshot.MAX_REREAD_SPAN',4):
            spans=list(_reread_spans({(10,4):b'abcd',(12,4):b'cdef',(16,2):b'gh'}))
        self.assertTrue(all(len(raw)<=4 for _,raw in spans))
        self.assertEqual({a+i:b for a,raw in spans for i,b in enumerate(raw)},dict(enumerate(b'abcdefgh',10)))

    def test_profile_input_and_budgets_reject_without_cached_proof(self):
        f = Fixture();f.profile['sha256'] = 'wrong'
        with self.assertRaises(SceneOwnerRejected):f.capture()
        f = Fixture();f.profile['field_super'] += 8
        with self.assertRaises(SceneOwnerRejected):f.capture()
        for invalid in (None, 123, True, 'wrong'):
            f = Fixture();f.profile['names_rva'] = invalid
            with self.assertRaises(SceneOwnerRejected):f.capture()
        f = Fixture();f.profile['name_entry_index'] = False
        with self.assertRaises(SceneOwnerRejected):f.capture()
        f = Fixture();f.base = (1 << 64) - 1
        with self.assertRaises(SceneOwnerRejected):f.capture()
        f = Fixture()
        with mock.patch('scene_owner_snapshot.MAX_READS', 8):
            with self.assertRaisesRegex(SceneOwnerRejected, 'budget'):f.capture()
        with mock.patch('scene_owner_snapshot.MAX_BYTES', 8):
            with self.assertRaisesRegex(SceneOwnerRejected, 'budget'):f.capture()
        with self.assertRaises(SceneOwnerRejected):capture_scene_owner(f, 0, f.scene, f.pc, f.local)


if __name__ == '__main__':
    unittest.main()
