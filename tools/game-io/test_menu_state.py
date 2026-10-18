"""Offline menu reader tests; no process handles or input are created."""
import struct
import unittest
from unittest import mock

from menu_state import MenuReader, registered, read_root_source_text, DialogStorageChanged, SnapshotChanged, InventoryChanged, RegistrySnapshotChanged


class FakeResolver:
    def __init__(self):
        self.pid = 123
        self.profile = {'struct_property_struct': 208, 'field_super': 96}
        self.objects_header, self.objects_data = 0x8000, 0x9000
        self.object_pointers = []
        self.identities, self.memory, self.props = {}, {}, {}
        self.ident_cache = {}
        self.next_address = 0x20000
        self.read_counts = {}
        self.change_on_second_read = None
        self.object(0x1000, 'Menu', 'Scene', 0)
        self.object(0x2000, 'Button', 'Widget', 0x1000)
        self.object(0x3000, 'GameUISceneClient', 'GameUISceneClient', 0x4000)
        self.object(0x4000, 'WarUIInteraction', 'WarUIInteraction', 0)
        self.object(0x5000, 'Enabled', 'UIState_Enabled', 0x1000)
        self.object(0x6000, 'Focused', 'UIState_Focused', 0x2000)
        self.put(self.objects_header, struct.pack('<Qii', self.objects_data, len(self.object_pointers), 32))
        self.scalar(0x4000, 'SceneClient', 'ObjectProperty', 0x3000, '<Q')
        self.scalar(0x4000, 'BlockUIInputSemaphore', 'IntProperty', 0, '<i')
        self.scalar(0x1000, 'SceneClient', 'ObjectProperty', 0x3000, '<Q')
        self.scalar(0x1000, 'LastPlayerIndex', 'IntProperty', 0, '<i')
        self.scalar(0x1000, 'SceneInputMode', 'ByteProperty', 4, '<B')
        self.scalar(0x1000, 'SceneTag', 'NameProperty', (1, 0), '<II')
        self.array(0x3000, 'ActiveScenes', [0x1000])
        self.array(0x1000, 'Children', [0x2000])
        self.array(0x2000, 'Children', [])
        self.array(0x1000, 'StateStack', [0x5000])
        self.array(0x2000, 'StateStack', [0x6000])
        self.array(0x1000, 'FocusControls', [0x2000], 'StructProperty')
        self.array(0x2000, 'FocusControls', [], 'StructProperty')
        self.focus_struct = self.allocate()
        self.object(self.focus_struct, 'PlayerInteractionData', 'ScriptStruct', 0)

    def allocate(self):
        result = self.next_address
        self.next_address += 0x200
        return result

    def put(self, address, raw):
        for offset, value in enumerate(raw):
            self.memory[address + offset] = value

    def object(self, address, name, cls, outer):
        index = len(self.object_pointers)
        self.object_pointers.append(address)
        self.put(self.objects_data + index * 8, struct.pack('<Q', address))
        self.identities[address] = dict(address=address, name=name, cls=cls, outer=outer,
                                        internal_index=index, number=0)
        self.put(address, bytes(0x60))

    def identity(self, address):
        if not address:
            return None
        self.read(address, 0x60)
        return self.identities[address]

    def class_name(self, obj):
        cls = obj['cls']
        return self.identities[cls]['name'] if isinstance(cls, int) else cls

    def property(self, owner, name, inherited=False):
        if isinstance(owner, int) and owner in self.identities and owner != self.focus_struct:
            owner = self.identities[owner]['name']
        if owner == self.focus_struct:
            return dict(address=0, offset={'FocusedControl': 0, 'LastFocusedControl': 8}[name],
                        type='ObjectProperty')
        if (owner, name) not in self.props:
            raise RuntimeError('Property ' + name + ' not found')
        prop = self.props[owner, name]
        if name == 'RenderBounds':
            prop = dict(prop, offset=struct.unpack('<I', self.read(prop['address'] + 0x94, 4))[0])
        return prop

    def scalar(self, address, name, kind, value, fmt):
        owner = self.class_name(self.identities[address])
        key = owner, name
        if key not in self.props:
            offset = 0x100 + sum(k[0] == owner for k in self.props) * 0x20
            self.props[key] = dict(address=self.allocate(), offset=offset, type=kind)
        values = value if isinstance(value, tuple) else (value,)
        self.put(address + self.props[key]['offset'], struct.pack(fmt, *values))

    def bounds(self, address, values=(0, 0, 1280, 720)):
        self.scalar(address, 'RenderBounds', 'FloatProperty', values, '<4f')
        prop = self.props[self.class_name(self.identities[address]), 'RenderBounds']
        self.put(prop['address'] + 0x70, struct.pack('<II', 4, 4))
        self.put(prop['address'] + 0x94, struct.pack('<I', prop['offset']))
        return prop

    def array(self, address, name, values, kind='ObjectProperty'):
        owner = self.class_name(self.identities[address])
        key = owner, name
        if key not in self.props:
            offset = 0x100 + sum(k[0] == owner for k in self.props) * 0x20
            prop_address, inner = self.allocate(), self.allocate()
            self.props[key] = dict(address=prop_address, offset=offset, type='ArrayProperty')
            self.object(inner, 'Inner', kind, prop_address)
            self.put(prop_address + 208, struct.pack('<Q', inner))
        data = self.allocate() if values else 0
        self.put(address + self.props[key]['offset'], struct.pack('<Qii', data, len(values), len(values)))
        if values:
            self.put(data, struct.pack('<' + 'Q' * len(values), *values))
        return data

    def struct_type(self, prop):
        if prop['address'] in getattr(self, 'detail_struct_types', {}):
            return self.u64(prop['address'] + self.profile['struct_property_struct'])
        return self.focus_struct

    def name(self, index):
        return 'MainMenu'

    def read(self, address, size):
        key = address, size
        self.read_counts[key] = self.read_counts.get(key, 0) + 1
        raw = bytes(self.memory[address + i] for i in range(size))
        if key == self.change_on_second_read and self.read_counts[key] >= 2:
            return bytes([raw[0] ^ 1]) + raw[1:]
        return raw

    def u64(self, address):
        return struct.unpack('<Q', self.read(address, 8))[0]

    def layout_classes(self):
        """Separate real-style UIScene and UIObject branches; no root bounds."""
        classes = {name:self.allocate() for name in
                   ('Class', 'Package', 'Object', 'UIRoot', 'UIScreenObject', 'UIScene', 'UIObject', 'Scene', 'Widget')}
        engine = self.allocate()
        self.object(engine, 'Engine', classes['Package'], 0)
        supers = dict(Class='Object', Package='Object', Object=None, UIRoot='Object',
                      UIScreenObject='UIRoot', UIScene='UIScreenObject', UIObject='UIScreenObject',
                      Scene='UIScene', Widget='UIObject')
        for name, address in classes.items():
            self.object(address, name, classes['Class'], engine)
            self.put(address + 96, struct.pack('<Q', classes[supers[name]] if supers[name] else 0))
        for obj in self.identities.values():
            if obj['cls'] in ('Scene', 'Widget'):obj['cls'] = classes[obj['cls']]
        self.put(self.objects_header, struct.pack('<Qii', self.objects_data, len(self.object_pointers), len(self.object_pointers)))
        self.layout_class_addresses, self.layout_engine = classes, engine
        return classes

    def layout_storage(self):
        """Synthetic reflected structs with deliberately non-native offsets."""
        classes = self.layout_classes()
        self.profile['property_offset_internal'] = 0x94
        self.detail_struct_types = {}
        structures = {}
        for name in ('UIScreenValue_Bounds', 'UIDockingSet', 'UIScreenValue_DockPadding'):
            structures[name] = self.allocate()
            self.object(structures[name], name, 'ScriptStruct', classes['UIRoot'])
        def prop(owner, name, kind, offset, dim, size, structure=None):
            address = self.allocate()
            self.object(address, name, kind, owner)
            self.props[self.identities[owner]['name'], name] = dict(address=address, offset=offset, type=kind)
            self.put(address + 0x70, struct.pack('<II', dim, size))
            self.put(address + 0x94, struct.pack('<I', offset))
            if structure is not None:
                self.detail_struct_types[address] = structures[structure]
                self.put(address + self.profile['struct_property_struct'], struct.pack('<Q', structures[structure]))
        prop(classes['UIScreenObject'], 'Position', 'StructProperty', 0x600, 1, 40, 'UIScreenValue_Bounds')
        for name,kind,offset,dim,size in (('Value','FloatProperty',8,4,4), ('ScaleType','ByteProperty',25,4,1),
                ('bInvalidated','ByteProperty',30,4,1), ('AspectRatioMode','ByteProperty',35,1,1)):
            prop(structures['UIScreenValue_Bounds'],name,kind,offset,dim,size)
        prop(classes['UIObject'], 'DockTargets', 'StructProperty', 0x700, 1, 112, 'UIDockingSet')
        for name,kind,offset,dim,size in (('OwnerWidget','ObjectProperty',8,1,8),
                ('TargetWidget','ObjectProperty',24,4,8), ('TargetFace','ByteProperty',60,4,1),
                ('bResolved','ByteProperty',64,4,1), ('bLinking','ByteProperty',68,4,1)):
            prop(structures['UIDockingSet'],name,kind,offset,dim,size)
        prop(structures['UIDockingSet'], 'DockPadding', 'StructProperty', 80, 1, 28, 'UIScreenValue_DockPadding')
        prop(structures['UIScreenValue_DockPadding'], 'PaddingValue', 'FloatProperty', 4, 4, 4)
        prop(structures['UIScreenValue_DockPadding'], 'PaddingScaleType', 'ByteProperty', 21, 4, 1)
        for address in (0x1000, 0x2000):
            self.put(address + 0x600, bytes(40))
            self.put(address + 0x608, struct.pack('<4f', 10, 20, 0.8, 0.9))
            self.put(address + 0x619, bytes((1, 2, 5, 6)))
            self.put(address + 0x61e, bytes((0, 1, 0, 1)))
        self.bounds(0x2000)
        self.put(0x2700, bytes(112))
        self.put(0x2708, struct.pack('<Q', 0x2000))
        self.put(0x2718, struct.pack('<4Q', 0x1000, 0, 0x2000, 0))
        self.put(0x273c, bytes((2, 4, 0, 4)))
        self.put(0x2754, struct.pack('<4f', 1, 2, 3, 4))
        self.put(0x2765, bytes((0, 1, 2, 4)))
        self.put(self.objects_header, struct.pack('<Qii', self.objects_data, len(self.object_pointers), len(self.object_pointers)))


class MenuStateTests(unittest.TestCase):
    def setUp(self):
        self.r = FakeResolver()
        self.reader = MenuReader.__new__(MenuReader)
        self.reader.r, self.reader.client, self.reader.properties = self.r, 0x3000, {}
        self.reader._registry_dirty = False
        # Focus on actual sample validation; no OS APIs or registry enumeration.
        self.reader.refresh = lambda: None

    def fail_scene_registration(self, change=None):
        original=self.r.identity
        armed=[True]
        def identity(address):
            obj=original(address)
            if address==0x1000 and armed[0]:
                armed[0]=False
                if change is not None:change()
                return dict(obj,internal_index=len(self.r.object_pointers)+17)
            return obj
        self.r.identity=identity

    def test_failed_registration_changed_header_is_dirty_and_preserves_cause(self):
        original_read=self.r.read
        def change():
            raw=self.r.read(self.r.objects_header,16)
            data,count,capacity=struct.unpack('<Qii',raw)
            self.r.put(self.r.objects_header,struct.pack('<Qii',data,count+1,capacity))
        self.fail_scene_registration(change)
        with self.assertRaises(RegistrySnapshotChanged) as caught:
            self.reader.sample(include_tree=True)
        self.assertIs(type(caught.exception),RegistrySnapshotChanged)
        self.assertEqual(str(caught.exception.__cause__),'Object outside registry')
        self.assertEqual(caught.exception.read_conflict['registry'],'objects')
        self.assertEqual(caught.exception.read_conflict['stage'],'menu_failed_registration_reread')
        self.assertTrue(self.reader._registry_dirty)
        self.assertEqual(self.r.read,original_read)
        self.assertFalse(hasattr(self.reader,'_dialog_raw_read'))

    def test_failed_registration_changed_active_scenes_source_is_snapshot_only(self):
        address=0x3000+self.r.props['GameUISceneClient','ActiveScenes']['offset']
        def change():
            raw=self.r.read(address,16);data,_,capacity=struct.unpack('<Qii',raw)
            self.r.put(address,struct.pack('<Qii',data,0,capacity))
        self.fail_scene_registration(change)
        with self.assertRaises(SnapshotChanged) as caught:self.reader._sample_once()
        self.assertIs(type(caught.exception),SnapshotChanged)
        self.assertEqual(caught.exception.read_conflict['address'],hex(address))
        self.assertFalse(self.reader._registry_dirty)
        # Entire failed acquisition was discarded; the next one independently
        # validates the now-empty menu rather than admitting the invalid object.
        result=self.reader.sample();self.assertTrue(result['consistent_reread'])
        self.assertEqual(result['active_scenes'],[])

    def test_failed_registration_stable_invalid_pointer_stays_fatal_once(self):
        self.fail_scene_registration()
        with mock.patch.object(self.reader,'_sample_once',wraps=self.reader._sample_once) as sample:
            with self.assertRaisesRegex(RuntimeError,'Object outside registry') as caught:self.reader.sample()
        self.assertIs(type(caught.exception),RuntimeError)
        self.assertEqual(sample.call_count,1)
        self.assertFalse(self.reader._registry_dirty)

    def test_failed_registration_reread_io_or_short_is_not_churn_authority(self):
        for short in (False,True):
            with self.subTest(short=short):
                self.setUp();original_read=self.r.read;failed=[False]
                def change():failed[0]=True
                def read(address,size):
                    if failed[0]:
                        if short:return b''
                        raise OSError('unavailable failure reread')
                    return original_read(address,size)
                self.r.read=read;self.fail_scene_registration(change)
                with self.assertRaisesRegex(RuntimeError,'Object outside registry') as caught:self.reader.sample()
                self.assertIs(type(caught.exception),RuntimeError)
                self.assertFalse(self.reader._registry_dirty)
                self.assertEqual(self.r.read,read)

    def test_other_failure_message_does_not_gain_failure_rereads(self):
        original=self.r.identity
        class Different(RuntimeError):pass
        for error in (RuntimeError('Object outside registry: unknown'),Different('Object outside registry')):
            self.setUp();original=self.r.identity
            def identity(address):
                if address==0x1000:raise error
                return original(address)
            self.r.identity=identity
            with self.assertRaises(type(error)) as caught:self.reader.sample()
            self.assertIs(caught.exception,error)
            self.assertEqual(self.r.read_counts[self.r.objects_header,16],1)

    def test_outside_registry_rejection_reports_same_identity_without_new_reads(self):
        import json
        for index in (len(self.r.object_pointers),0xffffffff):
            self.setUp();self.r.identities[0x1000]['internal_index']=index
            before=dict(self.r.read_counts)
            with self.assertRaisesRegex(RuntimeError,'Object outside registry') as caught:registered(self.r,0x1000)
            error=caught.exception;self.assertIs(type(error),RuntimeError)
            detail=json.loads(json.dumps(error.registration_failure))
            self.assertEqual(detail,dict(address='0x1000',item_none=False,item_empty=False,
                internal_index=index,inventory_length=len(self.r.object_pointers)))
            self.assertEqual(self.r.read_counts.get((0x1000,0x60),0)-before.get((0x1000,0x60),0),1)
            self.assertEqual(sum(self.r.read_counts.values())-sum(before.values()),1)

    def test_null_active_scene_stays_fatal_with_parseable_menu_detail(self):
        import json
        self.r.array(0x3000,'ActiveScenes',[0])
        with mock.patch.object(self.reader,'_sample_once',wraps=self.reader._sample_once) as sample:
            with self.assertRaisesRegex(RuntimeError,'Object outside registry') as caught:self.reader.sample()
        self.assertIs(type(caught.exception),RuntimeError);self.assertEqual(sample.call_count,1)
        detail=json.loads(json.dumps(caught.exception.read_conflict))
        self.assertEqual(detail['stage'],'menu_registration_rejected')
        self.assertEqual(detail['registration_failure'],dict(address='0x0',item_none=True,item_empty=True,
            internal_index=None,inventory_length=len(self.r.object_pointers)))
        self.assertEqual(detail['acquired_objects_header']['address'],hex(self.r.objects_header))
        self.assertEqual(detail['acquired_objects_header']['count'],6)
        self.assertFalse(self.reader._registry_dirty)

    def test_initializing_uint_index_stays_fatal_with_menu_detail(self):
        import json
        self.r.identities[0x1000]['internal_index']=0xffffffff
        with self.assertRaisesRegex(RuntimeError,'Object outside registry') as caught:self.reader.sample()
        self.assertIs(type(caught.exception),RuntimeError)
        detail=json.loads(json.dumps(caught.exception.read_conflict))
        self.assertEqual(detail['registration_failure']['internal_index'],4294967295)
        self.assertFalse(self.reader._registry_dirty)

    def test_changed_registration_preserves_detail_on_existing_churn_class(self):
        def change():
            raw=self.r.read(self.r.objects_header,16);data,count,capacity=struct.unpack('<Qii',raw)
            self.r.put(self.r.objects_header,struct.pack('<Qii',data,count+1,capacity))
        self.fail_scene_registration(change)
        with self.assertRaises(RegistrySnapshotChanged) as caught:self.reader.sample()
        self.assertEqual(caught.exception.read_conflict['registration_failure'],caught.exception.__cause__.registration_failure)
        self.assertEqual(caught.exception.registration_failure,caught.exception.__cause__.registration_failure)

    def test_owner_option_is_explicit_and_does_not_change_default_sample(self):
        with mock.patch('scene_owner_snapshot.capture_scene_owner', return_value={'render_admission': False}) as capture:
            baseline = self.reader.sample()
            self.assertNotIn('scene_owner', baseline)
            capture.assert_not_called()
            result = self.reader.sample(include_owner=True, expected_controller=0xa000, expected_local_player=0xb000)
            capture.assert_called_once_with(self.r, 0x3000, 0x1000, 0xa000, 0xb000)
            self.assertEqual(result['scene_owner'], {'render_admission': False})

    def test_owner_option_requires_addresses_before_any_read(self):
        for controller, player in ((None, None), (0, 1), (True, 2), (-1, 2), (2**64, 2), (1, '2')):
            with self.subTest(controller=controller, player=player):
                before = dict(self.r.read_counts)
                with self.assertRaises(ValueError):
                    self.reader.sample(include_owner=True, expected_controller=controller, expected_local_player=player)
                self.assertEqual(self.r.read_counts, before)

    def test_owner_failure_propagates_without_partial_sample_or_retry(self):
        from scene_owner_snapshot import SceneOwnerRejected
        original_read = self.r.read
        with mock.patch('scene_owner_snapshot.capture_scene_owner', side_effect=SceneOwnerRejected('changed')) as capture:
            with self.assertRaisesRegex(SceneOwnerRejected, 'changed'):
                self.reader.sample(include_owner=True, expected_controller=0xa000, expected_local_player=0xb000)
            self.assertEqual(capture.call_count, 1)
        self.assertEqual(self.r.read, original_read)

    def test_owner_option_rejects_missing_active_scene(self):
        self.r.array(0x3000, 'ActiveScenes', [])
        with mock.patch('scene_owner_snapshot.capture_scene_owner') as capture:
            with self.assertRaisesRegex(RuntimeError, 'active PathChoice'):
                self.reader.sample(include_owner=True, expected_controller=0xa000, expected_local_player=0xb000)
            capture.assert_not_called()

    def test_owner_cli_rejects_incomplete_addresses_before_opening_process(self):
        import menu_state
        for args in (['--owner'], ['--owner-controller', '0x1000'],
                     ['--owner', '--owner-controller', '0', '--owner-player', '0x2000']):
            with self.subTest(args=args), mock.patch('sys.argv', ['menu_state.py', '--pid', '123', *args]), \
                 mock.patch('menu_state.MenuReader') as reader, mock.patch('sys.stderr'):
                with self.assertRaises(SystemExit) as stopped:
                    menu_state.main()
                self.assertEqual(stopped.exception.code, 2)
                reader.assert_not_called()

    def add_tree_widget(self,address,parent,name='Branch'):
        self.r.object(address,name,'Widget',parent)
        self.r.array(address,'Children',[])
        self.r.array(address,'StateStack',[])
        self.r.array(address,'FocusControls',[],'StructProperty')

    def test_optional_layout_reads_all_nodes_and_preserves_normal_samples(self):
        self.add_tree_widget(0xa000, 0x2000, 'Descendant')
        self.r.array(0x2000, 'Children', [0xa000])
        self.r.layout_classes()
        baseline = self.reader.sample()
        expected = {0x2000:(1, 2, 300, 400), 0xa000:(10, 20, 100, 200)}
        for address, values in expected.items():
            prop = self.r.bounds(address, values)
            self.assertNotIn((address + prop['offset'], 16), self.r.read_counts)
        with mock.patch.object(self.r, 'property', wraps=self.r.property) as reflected:
            self.assertEqual(self.reader.sample(), baseline)
            self.reader.sample(include_tree=True)
            self.assertFalse(any(call.args[1] == 'RenderBounds' for call in reflected.call_args_list))
        result = self.reader.sample(include_layout=True)
        self.assertTrue(result['consistent_reread'])
        self.assertEqual(result['layout_evidence'], 'consistent CPU layout snapshot; not actual draw/scissor proof')
        self.assertEqual([row['address'] for row in result['tree']], ['0x1000', '0x2000', '0xa000'])
        root = result['tree'][0]
        self.assertIsNone(root['render_bounds'])
        self.assertIn('no UIObject ancestry', root['render_bounds_absence']['reason'])
        proof = root['render_bounds_absence']['proof']
        self.assertEqual(proof['base']['identity']['name'], 'UIScene')
        self.assertNotIn('UIObject', [c['identity']['name'] for c in proof['class_chain']])
        self.assertNotIn(('Scene', 'RenderBounds'), self.r.props)
        for row in result['tree'][1:]:
            bounds = row['render_bounds']
            values = expected[int(row['address'], 16)]
            self.assertEqual(bounds['values'], list(values))
            self.assertEqual(bounds['ordered_values'], list(values))
            self.assertFalse(bounds['x_reversed']);self.assertFalse(bounds['y_reversed'])
            self.assertEqual(bounds['order'], ['left', 'top', 'right', 'bottom'])
            self.assertEqual(bytes.fromhex(bounds['raw']), struct.pack('<4f', *values))
            self.assertEqual((bounds['reflected']['array_dim'], bounds['reflected']['element_size']), (4, 4))
            self.assertEqual(bounds['reflected']['type'], 'FloatProperty')

    def test_optional_layout_details_use_reflected_position_and_docking_extents(self):
        self.r.layout_storage()
        baseline = self.reader.sample(include_layout=True)
        with mock.patch.object(self.r, 'property', wraps=self.r.property) as reflected:
            self.assertEqual(self.reader.sample(include_layout=True), baseline)
            self.assertFalse(any(call.args[1] in ('Position','DockTargets') for call in reflected.call_args_list))
        result = self.reader.sample(include_layout_details=True)
        self.assertTrue(result['read_only']);self.assertTrue(result['consistent_reread'])
        root, child = result['tree']
        self.assertIn('layout_details', root)
        self.assertIsNone(root['layout_details']['docking'])
        self.assertIn('no UIObject ancestry', root['layout_details']['docking_absence'])
        details = child['layout_details']
        position = details['position']
        self.assertEqual(position['reflected']['offset'], 0x600)
        self.assertEqual(position['members']['Value']['values'][:2], [10, 20])
        self.assertEqual(position['members']['ScaleType']['values'], [1,2,5,6])
        self.assertEqual(position['members']['bInvalidated']['values'], [0,1,0,1])
        docking = details['docking']
        self.assertEqual(docking['members']['OwnerWidget']['values'][0]['address'], '0x2000')
        targets = docking['members']['TargetWidget']['values']
        self.assertEqual([value['address'] if value else None for value in targets], ['0x1000',None,'0x2000',None])
        self.assertEqual(docking['members']['TargetFace']['values'], [2,4,0,4])
        self.assertEqual(docking['padding']['members']['PaddingValue']['values'], [1,2,3,4])
        self.assertEqual(docking['padding']['members']['PaddingScaleType']['values'], [0,1,2,4])
        self.assertIn('bLockWidthWhenDocked', docking['omitted'])
        self.assertEqual(child['render_bounds'], baseline['tree'][1]['render_bounds'])
        self.assertNotIn('layout_details', self.reader.sample(include_layout=True)['tree'][1])

    def test_layout_details_reject_bad_metadata_extents_nonfinite_and_unregistered_dock_targets(self):
        for failure in ('dimension','size','offset','struct_owner','field_owner','type','nan','target'):
            with self.subTest(failure=failure):
                self.setUp();self.r.layout_storage()
                field = self.r.props['UIScreenValue_Bounds','Value']
                if failure == 'dimension':self.r.put(field['address']+0x70,struct.pack('<II',128,4))
                elif failure == 'size':self.r.put(field['address']+0x74,struct.pack('<I',8))
                elif failure == 'offset':
                    field['offset']=39;self.r.put(field['address']+0x94,struct.pack('<I',39))
                elif failure == 'struct_owner':
                    prop=self.r.props['UIScreenObject','Position']
                    self.r.identities[self.r.detail_struct_types[prop['address']]]['outer']=0x1000
                elif failure == 'field_owner':self.r.identities[field['address']]['outer']=0x1000
                elif failure == 'type':field['type']='ByteProperty'
                elif failure == 'nan':self.r.put(0x1608,struct.pack('<f',float('nan')))
                elif failure == 'target':self.r.put(0x2718,struct.pack('<Q',0xdeadbeef))
                with self.assertRaises((RuntimeError,KeyError)):
                    self.reader.sample(include_layout_details=True)

    def test_layout_detail_values_and_metadata_participate_in_existing_reread_ledger(self):
        for field_name in ('position','docking','metadata'):
            with self.subTest(field=field_name):
                self.setUp();self.r.layout_storage()
                key = (0x1608,16) if field_name=='position' else (0x2718,32)
                if field_name=='metadata':
                    key=(self.r.props['UIScreenValue_Bounds','Value']['address']+0x70,8)
                original=self.r.read
                def changing(address,size):
                    raw=original(address,size)
                    if (address,size)==key:
                        # Mutate after each observation so every snapshot's
                        # closing reread differs, including the existing retry.
                        self.r.put(address,bytes([raw[0]^1])+raw[1:])
                    return raw
                self.r.read=changing
                with self.assertRaises(SnapshotChanged):self.reader.sample(include_layout_details=True)

    def test_layout_rejects_unknown_malicious_unregistered_and_cyclic_ancestry(self):
        for failure in ('unknown_root', 'widget_root', 'scene_child', 'fake_metaclass',
                        'base_owner', 'unregistered_class', 'live_slot', 'cycle', 'mixed_branches'):
            with self.subTest(failure=failure):
                self.setUp()
                classes = self.r.layout_classes()
                self.r.bounds(0x2000)
                if failure == 'unknown_root':
                    self.r.put(classes['Scene'] + 96, struct.pack('<Q', classes['Object']))
                elif failure == 'widget_root':self.r.put(classes['Scene'] + 96, struct.pack('<Q', classes['UIObject']))
                elif failure == 'scene_child':self.r.put(classes['Widget'] + 96, struct.pack('<Q', classes['UIScene']))
                elif failure == 'fake_metaclass':self.r.identities[classes['UIScene']]['cls'] = classes['UIRoot']
                elif failure == 'base_owner':self.r.identities[classes['UIScene']]['outer'] = 0x3000
                elif failure == 'unregistered_class':
                    self.r.identities[classes['UIScene']]['internal_index'] = len(self.r.object_pointers)
                elif failure == 'live_slot':
                    index = self.r.identities[classes['UIScene']]['internal_index']
                    self.r.put(self.r.objects_data + index * 8, bytes(8))
                elif failure == 'cycle':self.r.put(classes['UIScene'] + 96, struct.pack('<Q', classes['Scene']))
                elif failure == 'mixed_branches':self.r.put(classes['UIScene'] + 96, struct.pack('<Q', classes['UIObject']))
                with self.assertRaisesRegex(RuntimeError, 'layout|registry'):
                    self.reader.sample(include_layout=True)

    def test_layout_changed_ancestry_is_in_snapshot_ledger_and_rejects_both_attempts(self):
        for field in ('super', 'identity'):
            with self.subTest(field=field):
                self.setUp()
                classes = self.r.layout_classes()
                self.r.bounds(0x2000)
                target = (classes['UIScene'] + 96, 8) if field == 'super' else (classes['UIScene'], 0x60)
                original = self.r.read
                def changing(address, size):
                    raw = original(address, size)
                    if (address, size) == target and self.r.read_counts[target] % 2 == 0:
                        return (struct.pack('<Q', classes['Object']) if field == 'super'
                                else bytes([raw[0] ^ 1]) + raw[1:])
                    return raw
                self.r.read = changing
                with mock.patch.object(self.reader, '_sample_once', wraps=self.reader._sample_once) as sampled:
                    with self.assertRaises(SnapshotChanged):self.reader.sample(include_layout=True)
                    self.assertEqual(sampled.call_count, 2)

    def test_layout_explicitly_rejects_unknown_or_invalid_metadata_and_bounds(self):
        for failure in ('missing', 'type', 'dimension', 'element', 'offset', 'nan', 'infinite'):
            with self.subTest(failure=failure):
                self.setUp()
                self.r.layout_classes()
                if failure != 'missing':
                    self.r.bounds(0x2000)
                    prop = self.r.props['Widget', 'RenderBounds']
                    if failure == 'type':prop['type'] = 'StructProperty'
                    elif failure == 'dimension':self.r.put(prop['address'] + 0x70, struct.pack('<I', 3))
                    elif failure == 'element':self.r.put(prop['address'] + 0x74, struct.pack('<I', 8))
                    elif failure == 'offset':self.r.put(prop['address'] + 0x94, struct.pack('<I', 0x10000 - 15))
                    else:
                        values = {'nan':(float('nan'),0,1,1), 'infinite':(0,0,float('inf'),1)}[failure]
                        self.r.put(0x2000 + prop['offset'], struct.pack('<4f', *values))
                with self.assertRaises(RuntimeError):self.reader.sample(include_layout=True)
                self.assertTrue(self.reader.sample()['consistent_reread'])

    def test_layout_preserves_directed_edges_for_reversed_axes(self):
        cases = [((30, 2, 10, 20), True, False, [10, 2, 30, 20]),
                 ((1, 40, 30, 20), False, True, [1, 20, 30, 40]),
                 ((30, 40, 10, 20), True, True, [10, 20, 30, 40]),
                 ((1, 2, 1, 2), False, False, [1, 2, 1, 2]),
                 ((1331.8120, 148.3923, 1281.8113, 248.3918), True, False, None)]
        for values, reversed_x, reversed_y, ordered in cases:
            with self.subTest(values=values):
                self.setUp();self.r.layout_classes()
                self.r.bounds(0x2000, values)
                result = self.reader.sample(include_layout=True)
                bounds = result['tree'][1]['render_bounds']
                raw = struct.pack('<4f', *values)
                actual = list(struct.unpack('<4f', raw))
                self.assertTrue(result['consistent_reread'])
                self.assertEqual(bounds['values'], actual)
                self.assertEqual(bounds['raw'], raw.hex())
                self.assertEqual(bounds['order'], ['left', 'top', 'right', 'bottom'])
                self.assertIs(bounds['x_reversed'], reversed_x)
                self.assertIs(bounds['y_reversed'], reversed_y)
                if ordered is None:ordered = [actual[2], actual[1], actual[0], actual[3]]
                self.assertEqual(bounds['ordered_values'], ordered)

    def test_reversed_layout_still_rejects_nonfinite_values_on_every_edge(self):
        for index in range(4):
            for invalid in (float('nan'), float('inf'), -float('inf')):
                with self.subTest(edge=index, invalid=invalid):
                    self.setUp();self.r.layout_classes()
                    values = [30, 40, 10, 20];values[index] = invalid
                    self.r.bounds(0x2000, tuple(values))
                    with self.assertRaisesRegex(RuntimeError, 'Non-finite'):
                        self.reader.sample(include_layout=True)

    def test_layout_metadata_and_late_bounds_mutations_reject_both_attempts(self):
        for kind in ('bounds', 'dimension', 'offset'):
            with self.subTest(kind=kind):
                self.setUp()
                self.r.layout_classes()
                self.r.bounds(0x2000)
                prop = self.r.props['Widget', 'RenderBounds']
                target = {'bounds':(0x2000 + prop['offset'],16),
                          'dimension':(prop['address'] + 0x70,8), 'offset':(prop['address'] + 0x94,4)}[kind]
                original = self.r.read
                def changing(address, size):
                    raw = original(address, size)
                    if (address, size) == target and self.r.read_counts[target] % 2 == 0:
                        return bytes([raw[0] ^ 1]) + raw[1:]
                    return raw
                self.r.read = changing
                with mock.patch.object(self.reader, '_sample_once', wraps=self.reader._sample_once) as sampled:
                    with self.assertRaises(SnapshotChanged):self.reader.sample(include_layout=True)
                    self.assertEqual(sampled.call_count, 2)
                self.assertIs(self.r.read, changing)

    def test_late_bounds_change_discards_old_layout_before_recovery(self):
        self.r.layout_classes()
        self.r.bounds(0x2000)
        prop = self.r.props['Widget', 'RenderBounds']
        target = (0x2000 + prop['offset'], 16)
        original = self.r.read
        def changing(address, size):
            if (address, size) == target and self.r.read_counts.get(target, 0) == 1:
                self.r.put(address, struct.pack('<4f', 10, 20, 100, 200))
            return original(address, size)
        self.r.read = changing
        with mock.patch.object(self.reader, '_sample_once', wraps=self.reader._sample_once) as sampled:
            result = self.reader.sample(include_layout=True)
        self.assertEqual(sampled.call_count, 2)
        self.assertEqual(result['tree'][1]['render_bounds']['values'], [10, 20, 100, 200])

    def test_layout_cli_implies_tree(self):
        import menu_state
        reader = mock.Mock()
        reader.sample.return_value = dict(pid=123, tree=[])
        with mock.patch.object(menu_state, 'MenuReader', return_value=reader), \
             mock.patch.object(menu_state.sys, 'argv', ['menu_state.py', '--pid', '123', '--layout']), \
             mock.patch('builtins.print'):
            menu_state.main()
        reader.sample.assert_called_once_with(include_tree=True, include_layout=True)

    def test_layout_details_cli_is_explicit_and_implies_layout_tree(self):
        import menu_state
        reader = mock.Mock()
        reader.sample.return_value = dict(pid=123, tree=[])
        with mock.patch.object(menu_state, 'MenuReader', return_value=reader), \
                mock.patch.object(menu_state.sys, 'argv', ['menu_state.py', '--pid', '123', '--layout-details']), \
                mock.patch('builtins.print'):
            menu_state.main()
        reader.sample.assert_called_once_with(include_tree=True, include_layout=True, include_layout_details=True)
        reader.close.assert_called_once()

    def test_optional_tree_includes_unfocused_descendants_without_default_reads(self):
        self.add_tree_widget(0xa000,0x1000,'OptionsCandidate')
        self.add_tree_widget(0xb000,0xa000,'NestedCandidate')
        self.r.array(0x1000,'Children',[0x2000,0xa000])
        self.r.array(0xa000,'Children',[0xb000])
        self.r.object(0xc000,'Disabled','UIState_Disabled',0xa000)
        self.r.array(0xa000,'StateStack',[0xc000])
        self.assertNotIn('tree',self.reader.sample())
        off_path=(0xa000+self.r.props['Widget','StateStack']['offset'],16)
        self.assertNotIn(off_path,self.r.read_counts)
        result=self.reader.sample(include_tree=True)
        self.assertTrue(result['consistent_reread'])
        rows=result['tree']
        self.assertEqual([r['name'] for r in rows],['Menu','Button','OptionsCandidate','NestedCandidate'])
        self.assertEqual(rows[0]['parent'],None)
        self.assertEqual(rows[0]['focus_child'],'0x2000')
        self.assertEqual((rows[2]['parent'],rows[2]['states']),('0x1000',['UIState_Disabled']))
        self.assertEqual((rows[3]['parent'],rows[3]['depth']),('0xa000',3))

    def test_unfocused_tree_changes_rejected_by_final_reread(self):
        self.add_tree_widget(0xa000,0x1000)
        self.r.array(0x1000,'Children',[0x2000,0xa000])
        offset=self.r.props['Widget','Children']['offset']
        target=(0xa000+offset,16);original=self.r.read
        def unstable(address,size):
            raw=original(address,size)
            if (address,size)==target and self.r.read_counts[target]%2==0:
                return bytes([raw[0]^1])+raw[1:]
            return raw
        self.r.read=unstable
        with mock.patch.object(self.reader,'_sample_once',wraps=self.reader._sample_once) as sampled:
            with self.assertRaisesRegex(SnapshotChanged,'UI changed before snapshot verification'):
                self.reader.sample(include_tree=True)
            self.assertEqual(sampled.call_count,2)
        self.assertIs(self.r.read,unstable)

    def test_unfocused_tree_rejects_bad_ownership_focus_and_cycles(self):
        for failure in ('outer','state_outer','focus_membership','cycle','duplicate'):
            with self.subTest(failure=failure):
                self.setUp();self.add_tree_widget(0xa000,0x1000)
                self.r.array(0x1000,'Children',[0x2000,0xa000])
                if failure=='outer':self.r.identities[0xa000]['outer']=0x3000
                elif failure=='state_outer':self.r.array(0xa000,'StateStack',[0x6000])
                elif failure=='focus_membership':self.r.array(0xa000,'FocusControls',[0x2000],'StructProperty')
                elif failure=='cycle':self.r.array(0xa000,'Children',[0x1000])
                else:self.r.array(0xa000,'Children',[0x2000])
                with self.assertRaises(RuntimeError):self.reader.sample(include_tree=True)

    def test_tree_node_and_depth_boundaries(self):
        for nodes in (128,129):
            with self.subTest(nodes=nodes):
                self.setUp();children=[0x2000]
                for i in range(nodes-2):
                    address=0x100000+i*0x1000;self.add_tree_widget(address,0x1000);children.append(address)
                self.r.array(0x1000,'Children',children)
                if nodes==128:self.assertEqual(len(self.reader.sample(include_tree=True)['tree']),128)
                else:
                    with self.assertRaisesRegex(RuntimeError,'node/depth limit'):self.reader.sample(include_tree=True)
        for depth in (16,17):
            with self.subTest(depth=depth):
                self.setUp();parent=0x1000
                for i in range(depth-1):
                    address=0x100000+i*0x1000;self.add_tree_widget(address,parent)
                    self.r.array(parent,'Children',([0x2000] if parent==0x1000 else [])+[address]);parent=address
                if depth==16:self.assertEqual(max(r['depth'] for r in self.reader.sample(include_tree=True)['tree']),16)
                else:
                    with self.assertRaisesRegex(RuntimeError,'node/depth limit'):self.reader.sample(include_tree=True)

    def test_valid_focus_chain_and_restore_read_method(self):
        result = self.reader.sample()
        self.assertEqual(result['top']['name'], 'Menu')
        self.assertEqual(result['active_scenes'][0]['input_mode'], 4)
        self.assertEqual(result['focused_leaf']['name'], 'Button')
        self.assertEqual(result['focused_leaf']['states'], ['UIState_Focused'])
        self.assertTrue(result['consistent_reread'])
        self.assertEqual(self.r.read.__func__, FakeResolver.read)

    def test_array_invalid_counts_and_null_storage(self):
        obj = self.r.identities[0x3000]
        prop = self.r.props['GameUISceneClient', 'ActiveScenes']
        for data, count, capacity in ((1, -1, 1), (1, 2, 1), (1, 0, 129), (0, 1, 1)):
            with self.subTest(count=count, capacity=capacity, data=data):
                self.r.put(obj['address'] + prop['offset'], struct.pack('<Qii', data, count, capacity))
                with self.assertRaisesRegex(RuntimeError, 'Invalid bounded array'):
                    self.reader.objects(obj, 'ActiveScenes')

    def test_focus_array_requires_struct_inner(self):
        prop = self.r.props['Scene', 'FocusControls']
        inner = self.r.u64(prop['address'] + 208)
        self.r.identities[inner]['cls'] = 'ObjectProperty'
        with self.assertRaisesRegex(RuntimeError, 'Array inner metadata mismatch'):
            self.reader.sample()

    def test_focus_cycle(self):
        self.r.array(0x2000, 'FocusControls', [0x1000], 'StructProperty')
        self.r.array(0x2000, 'Children', [0x1000])
        self.r.identities[0x1000]['outer'] = 0x2000
        with self.assertRaisesRegex(RuntimeError, 'Focus cycle/depth limit'):
            self.reader.sample()

    def test_focused_child_not_in_children(self):
        self.r.array(0x1000, 'Children', [])
        with self.assertRaisesRegex(RuntimeError, 'Focused child outside parent tree'):
            self.reader.sample()

    def test_focused_child_wrong_outer(self):
        self.r.identities[0x2000]['outer'] = 0x3000
        with self.assertRaisesRegex(RuntimeError, 'Focused child outside parent tree'):
            self.reader.sample()

    def test_state_wrong_owner(self):
        self.r.identities[0x6000]['outer'] = 0x1000
        with self.assertRaisesRegex(RuntimeError, 'UI state owner mismatch'):
            self.reader.sample()

    def test_changed_value_reread_discards_first_snapshot_and_restores(self):
        offset = self.r.props['WarUIInteraction', 'BlockUIInputSemaphore']['offset']
        self.r.change_on_second_read = 0x4000 + offset, 4
        with mock.patch.object(self.reader,'_sample_once',wraps=self.reader._sample_once) as sampled,\
             mock.patch.object(self.reader,'force_registry_refresh') as forced:
            result=self.reader.sample()
            self.assertEqual(sampled.call_count,2);forced.assert_not_called()
        self.assertEqual(result['input_block_count'],1)
        self.assertTrue(result['consistent_reread'])
        self.assertEqual(self.r.read.__func__, FakeResolver.read)

    def test_during_snapshot_read_race_also_restarts_whole_snapshot(self):
        self.r.change_on_second_read=(0x2000,0x60)
        with mock.patch.object(self.reader,'_sample_once',wraps=self.reader._sample_once) as sampled:
            result=self.reader.sample(include_tree=True)
            self.assertEqual(sampled.call_count,2)
        self.assertTrue(result['consistent_reread'])
        self.assertEqual(len(result['tree']),2)
        self.assertEqual(self.r.read.__func__,FakeResolver.read)

    def test_snapshot_race_and_registry_race_share_total_two_attempts(self):
        errors=[SnapshotChanged('UI changed during snapshot'),RuntimeError('Object registry identity changed')]
        with mock.patch.object(self.reader,'_sample_once',side_effect=errors) as sampled,\
             mock.patch.object(self.reader,'force_registry_refresh') as forced:
            with self.assertRaisesRegex(InventoryChanged,'Object registry identity changed'):self.reader.sample()
            self.assertEqual(sampled.call_count,2);forced.assert_not_called()
        with mock.patch.object(self.reader,'_sample_once',side_effect=list(reversed(errors))) as sampled,\
             mock.patch.object(self.reader,'force_registry_refresh') as forced:
            with self.assertRaises(SnapshotChanged):self.reader.sample()
            self.assertEqual(sampled.call_count,2);forced.assert_called_once()

    def test_snapshot_message_without_exact_type_does_not_retry(self):
        class OtherSnapshotError(SnapshotChanged):pass
        for error in (RuntimeError('UI changed during snapshot'),OtherSnapshotError('UI changed during snapshot')):
            with mock.patch.object(self.reader,'_sample_once',side_effect=error) as sampled:
                with self.assertRaises(type(error)):self.reader.sample()
                sampled.assert_called_once()

    def test_second_cached_inventory_mismatch_is_typed_without_a_third_attempt(self):
        for first in (RuntimeError('Object registry identity changed'),SnapshotChanged('changed')):
            second=RuntimeError('Object registry identity changed')
            with self.subTest(first=type(first).__name__),\
                 mock.patch.object(self.reader,'_sample_once',side_effect=[first,second]) as sampled,\
                 mock.patch.object(self.reader,'force_registry_refresh') as forced:
                with self.assertRaises(InventoryChanged) as raised:self.reader.sample(include_tree=True)
                self.assertIs(type(raised.exception),InventoryChanged);self.assertIs(raised.exception.__cause__,second)
                self.assertEqual(sampled.call_count,2)
                self.assertEqual(sampled.call_args_list,[mock.call(include_tree=True)]*2)
                self.assertEqual(forced.call_count,int(type(first) is RuntimeError))

    def test_force_refresh_error_never_becomes_snapshot_inventory_churn(self):
        for error in (RuntimeError('Object registry identity changed'),RuntimeError('Registry header changed'),OSError('RPM failed')):
            with self.subTest(error=repr(error)),\
                 mock.patch.object(self.reader,'_sample_once',side_effect=RuntimeError('Object registry identity changed')) as sampled,\
                 mock.patch.object(self.reader,'force_registry_refresh',side_effect=error) as forced:
                with self.assertRaises(type(error)) as raised:self.reader.sample()
                self.assertIs(raised.exception,error);self.assertNotIsInstance(raised.exception,InventoryChanged)
                sampled.assert_called_once();forced.assert_called_once()

    def test_second_non_cached_identity_header_client_and_io_failures_stay_original(self):
        class DifferentFailure(RuntimeError):pass
        for error in (RuntimeError('Live object registry slot changed'),RuntimeError('Scene client replaced'),
                      RuntimeError('Registry changed before snapshot verification'),
                      RuntimeError('Registry header changed during forced refresh'),
                      RuntimeError('Object registry identity changed: unknown'),
                      DifferentFailure('Object registry identity changed'),OSError('RPM failed'),ValueError('bad header')):
            with self.subTest(error=repr(error)),\
                 mock.patch.object(self.reader,'_sample_once',side_effect=[SnapshotChanged('changed'),error]) as sampled,\
                 mock.patch.object(self.reader,'force_registry_refresh') as forced:
                with self.assertRaises(type(error)) as raised:self.reader.sample()
                self.assertIs(raised.exception,error);self.assertNotIsInstance(raised.exception,InventoryChanged)
                self.assertEqual(sampled.call_count,2);forced.assert_not_called()

    def test_live_slot_replacement_rejected(self):
        obj = self.r.identities[0x2000]
        self.r.put(self.r.objects_data + obj['internal_index'] * 8, struct.pack('<Q', 0xdead))
        with self.assertRaisesRegex(RuntimeError, 'Live object registry slot changed'):
            registered(self.r, 0x2000)

    def test_nonzero_player_is_reported_not_accepted_as_zero(self):
        self.r.scalar(0x1000, 'LastPlayerIndex', 'IntProperty', 1, '<i')
        self.assertEqual(self.reader.sample()['player_index'], 1)


class RegistryRecoveryTests(unittest.TestCase):
    def setUp(self):
        MenuStateTests.setUp(self)
        self.reader.client_identity=dict(self.r.identities[self.reader.client])
        self.r.names_header,self.r.names_data=0x8200,0x400000
        self.r.name_count=self.r.name_capacity=1001
        self.r.name_pointers=(0,)*1001
        self.r.names_cache={'stale':'must clear'}
        self.r.put(self.r.names_header,struct.pack('<Qii',self.r.names_data,1001,1001))
        self.r.put(self.r.names_data,bytes(1001*8))
        pointers=self.r.object_pointers+[0]*(1001-len(self.r.object_pointers))
        self.r.object_pointers=pointers
        self.r.object_count=self.r.object_capacity=1001
        self.r.put(self.r.objects_header,struct.pack('<Qii',self.r.objects_data,1001,1001))
        self.r.put(self.r.objects_data,struct.pack('<'+'Q'*1001,*pointers))
        self.r.name=lambda index:'None' if index==0 else 'MainMenu'
        self.r.prepare=mock.Mock(side_effect=AssertionError('Must not repeat prepare'))
        self.slot=self.r.identities[0x2000]['internal_index']
        # Stable registry headers, stale cached zero, valid current live slot.
        self.r.object_pointers[self.slot]=0

    def arm_final_header_change(self):
        # First header read is ledger acquisition, second is ledger validation;
        # the third is the distinct final bracket after every ledger read passed.
        self.r.object_pointers[self.slot]=0x2000
        original=self.r.read
        counter={'reads':0,'changes':0}
        def read(address,size):
            raw=original(address,size)
            if (address,size)==(self.r.objects_header,16):
                counter['reads']+=1
                if counter['reads']==3:
                    data,count,capacity=struct.unpack('<Qii',raw)
                    raw=struct.pack('<Qii',data,count,capacity+1)
                    self.r.put(address,raw);counter['changes']+=1
            return raw
        self.r.read=read
        return counter,read

    def test_real_final_third_header_race_defers_refresh_until_next_sample(self):
        counter,read=self.arm_final_header_change()
        with mock.patch.object(self.reader,'_sample_once',wraps=self.reader._sample_once) as sampled,\
             mock.patch.object(self.reader,'force_registry_refresh',wraps=self.reader.force_registry_refresh) as forced:
            with self.assertRaises(RegistrySnapshotChanged) as raised:self.reader.sample(include_tree=True)
            self.assertIs(type(raised.exception),RegistrySnapshotChanged)
            self.assertNotIsInstance(raised.exception,SnapshotChanged)
            detail=raised.exception.read_conflict
            self.assertEqual(detail['stage'],'menu_registry_bracket')
            self.assertEqual(detail['address'],hex(self.r.objects_header))
            self.assertEqual(detail['registry'],'objects')
            self.assertEqual(len(bytes.fromhex(detail['initial_raw'])),16)
            self.assertEqual(detail['observed']['max'],detail['initial']['max']+1)
            self.assertEqual(detail['observed']['data'],detail['initial']['data'])
            self.assertEqual(detail['observed']['num'],detail['initial']['num'])
            self.assertTrue(self.reader._registry_dirty);self.assertEqual(counter['reads'],3)
            sampled.assert_called_once_with(include_tree=True);forced.assert_not_called()
            self.assertIs(self.r.read,read);self.assertFalse(hasattr(self.reader,'_dialog_raw_read'))
            result=self.reader.sample(include_tree=True)
            forced.assert_called_once();self.assertEqual(sampled.call_count,2)
        self.assertTrue(result['consistent_reread']);self.assertFalse(self.reader._registry_dirty)
        self.assertEqual(self.r.object_pointers[self.slot],0x2000)
        self.assertNotIn('stale',self.r.names_cache);self.r.prepare.assert_not_called()

    def test_repeated_real_final_races_stay_dirty_without_nested_retry(self):
        counter,read=self.arm_final_header_change()
        with self.assertRaises(RegistrySnapshotChanged):self.reader.sample()
        original=self.reader.force_registry_refresh
        def refreshed():
            original();counter['reads']=0
        with mock.patch.object(self.reader,'force_registry_refresh',side_effect=refreshed) as forced,\
             mock.patch.object(self.reader,'_sample_once',wraps=self.reader._sample_once) as sampled:
            with self.assertRaises(RegistrySnapshotChanged):self.reader.sample()
            forced.assert_called_once();sampled.assert_called_once()
        self.assertEqual(counter['changes'],2);self.assertTrue(self.reader._registry_dirty)
        self.assertIs(self.r.read,read);self.r.prepare.assert_not_called()

    def test_dirty_refresh_failures_keep_original_exception_and_dirty_without_sampling(self):
        for fault in ('client','invalid_header','changing_header','io'):
            with self.subTest(fault=fault):
                self.setUp();self.reader._registry_dirty=True
                if fault=='client':self.r.identities[self.reader.client]['number']+=1
                elif fault=='invalid_header':self.r.put(self.r.names_header,struct.pack('<Qii',self.r.names_data,1000,1001))
                elif fault=='changing_header':self.r.change_on_second_read=(self.r.names_header,16)
                else:
                    error=OSError('read failed');self.r.read=mock.Mock(side_effect=error)
                expected={'client':'Scene client identity changed','invalid_header':'Invalid forced registry header',
                          'changing_header':'Registry header changed during forced refresh','io':'read failed'}[fault]
                with mock.patch.object(self.reader,'_sample_once') as sampled,\
                     mock.patch.object(self.reader,'force_registry_refresh',wraps=self.reader.force_registry_refresh) as forced:
                    with self.assertRaisesRegex((RuntimeError,OSError),expected) as raised:self.reader.sample()
                    if fault=='io':self.assertIs(raised.exception,error)
                    self.assertNotIsInstance(raised.exception,(RegistrySnapshotChanged,InventoryChanged))
                    forced.assert_called_once();sampled.assert_not_called()
                self.assertTrue(self.reader._registry_dirty);self.r.prepare.assert_not_called()

    def test_dirty_and_mixed_races_share_one_refresh_and_two_sample_budget(self):
        cached=lambda:RuntimeError('Object registry identity changed')
        sequences=([cached()], [SnapshotChanged('changed'),cached()],
                   [DialogStorageChanged('changed'),cached()],
                   [SnapshotChanged('changed'),SnapshotChanged('changed')],
                   [DialogStorageChanged('changed'),{'consistent_reread':True}])
        for sequence in sequences:
            with self.subTest(sequence=sequence):
                self.reader._registry_dirty=True
                with mock.patch.object(self.reader,'force_registry_refresh') as forced,\
                     mock.patch.object(self.reader,'_sample_once',side_effect=sequence) as sampled:
                    if isinstance(sequence[-1],dict):self.assertTrue(self.reader.sample()['consistent_reread'])
                    else:
                        expected=InventoryChanged if str(sequence[-1])=='Object registry identity changed' else type(sequence[-1])
                        with self.assertRaises(expected):self.reader.sample()
                    forced.assert_called_once();self.assertEqual(sampled.call_count,len(sequence))
                    self.assertLessEqual(sampled.call_count,2)
        # A first ordinary UI race followed by the final-header type never gets
        # a third snapshot, nor an in-call refresh of the dirty failed sample.
        for first in (SnapshotChanged('ledger'),cached()):
            self.reader._registry_dirty=False
            def second_final(*,include_tree=False):
                if second_final.calls==0:
                    second_final.calls+=1;raise first
                self.reader._registry_dirty=True;raise RegistrySnapshotChanged('final header')
            second_final.calls=0
            with mock.patch.object(self.reader,'_sample_once',side_effect=second_final) as sampled,\
                 mock.patch.object(self.reader,'force_registry_refresh') as forced:
                with self.assertRaises(RegistrySnapshotChanged):self.reader.sample()
                self.assertEqual(sampled.call_count,2)
                self.assertEqual(forced.call_count,int(type(first) is RuntimeError))
            self.assertTrue(self.reader._registry_dirty)

    def test_same_header_slot_replacement_recovers_without_prepare_or_client_scan(self):
        before={p:self.r.read(getattr(self.r,p+'_header'),16) for p in ('names','objects')}
        with mock.patch.object(self.reader,'force_registry_refresh',wraps=self.reader.force_registry_refresh) as forced,\
             mock.patch.object(self.r,'identity',wraps=self.r.identity) as identities:
            result=self.reader.sample(include_tree=True)
            forced.assert_called_once()
            self.assertLess(identities.call_count,100)  # Inventory is not scanned object-by-object.
        self.assertTrue(result['consistent_reread'])
        self.assertEqual(result['focused_leaf']['address'],'0x2000')
        self.assertEqual(len(result['tree']),2)
        self.assertEqual(self.r.object_pointers[self.slot],0x2000)
        self.assertNotIn('stale',self.r.names_cache)
        self.r.prepare.assert_not_called()
        self.assertEqual(before,{p:self.r.read(getattr(self.r,p+'_header'),16) for p in before})
        self.assertEqual(self.r.read.__func__,FakeResolver.read)

    def test_real_live_slot_mismatch_still_rejected_after_one_refresh(self):
        self.r.put(self.r.objects_data+self.slot*8,struct.pack('<Q',0xdead))
        with mock.patch.object(self.reader,'force_registry_refresh',wraps=self.reader.force_registry_refresh) as forced:
            with self.assertRaisesRegex(RuntimeError,'Object registry identity changed'):
                self.reader.sample()
            forced.assert_called_once()
        self.r.prepare.assert_not_called()

    def test_original_client_identity_must_remain_unchanged(self):
        self.r.identities[self.reader.client]['number']+=1
        with self.assertRaisesRegex(RuntimeError,'Scene client identity changed'):
            self.reader.sample()

    def arm_forced_refresh_suffix(self,prefix,change='num'):
        for name in ('names','objects'):
            address=getattr(self.r,name+'_header')
            data,num,_=struct.unpack('<Qii',self.r.read(address,16))
            self.r.put(address,struct.pack('<Qii',data,num,2000))
        source=self.r.read;calls={'names':0,'objects':0}
        def read(address,size):
            raw=source(address,size)
            for name in calls:
                if (address,size)==(getattr(self.r,name+'_header'),16):
                    calls[name]+=1
                    if name==prefix and calls[name]==2:
                        data,num,capacity=struct.unpack('<Qii',raw)
                        changes={'num':(data,num+3,capacity),'shrink':(data,num-1,capacity),
                            'data':(data+8,num,capacity),'max':(data,num,capacity+1),
                            'excess':(data,capacity+1,capacity)}
                        return struct.pack('<Qii',*changes[change])
            return raw
        self.r.read=read
        return calls

    def test_explicit_forced_prefix_accepts_growth_without_adopting_suffix(self):
        for prefix in ('names','objects'):
            with self.subTest(prefix=prefix):
                self.setUp();calls=self.arm_forced_refresh_suffix(prefix)
                self.reader.force_registry_refresh(registry_header_policy='fixed_prefix_v1')
                self.assertEqual(calls,dict(names=2,objects=2))
                self.assertEqual(self.r.object_count,1001)
                self.assertEqual(self.r.name_count,1001)
                self.assertEqual(len(self.r.object_pointers),1001)
                self.assertEqual(len(self.r.name_pointers),1001)
                self.assertEqual(self.reader.client_identity,self.r.identities[self.reader.client])

    def test_explicit_forced_prefix_rejects_invalid_boundaries_and_old_identity(self):
        from menu_state import ForcedRegistryRefreshChanged
        for prefix in ('names','objects'):
            for change in ('shrink','data','max','excess'):
                with self.subTest(prefix=prefix,change=change):
                    self.setUp();self.arm_forced_refresh_suffix(prefix,change)
                    with self.assertRaises(ForcedRegistryRefreshChanged):
                        self.reader.force_registry_refresh(registry_header_policy='fixed_prefix_v1')
        for change in ('slot','client','name_zero'):
            with self.subTest(change=change):
                self.setUp();self.arm_forced_refresh_suffix('objects')
                if change=='slot':
                    index=self.r.identities[self.reader.client]['internal_index']
                    self.r.put(self.r.objects_data+index*8,struct.pack('<Q',0))
                elif change=='client':self.r.identities[self.reader.client]['number']+=1
                else:self.r.name=lambda index:'changed'
                with self.assertRaises(RuntimeError):
                    self.reader.force_registry_refresh(registry_header_policy='fixed_prefix_v1')
        with self.assertRaisesRegex(ValueError,'Unsupported forced registry'):
            self.reader.force_registry_refresh(registry_header_policy='unknown')

    def test_forced_refresh_reports_same_read_header_fields_and_short_circuits(self):
        from menu_state import ForcedRegistryRefreshChanged
        for prefix in ('names','objects'):
            for change in ('num','data','max'):
                with self.subTest(prefix=prefix,change=change):
                    self.setUp()
                    for name in ('names','objects'):
                        pointer=getattr(self.r,name+'_header')
                        data,count,_=struct.unpack('<Qii',self.r.read(pointer,16))
                        self.r.put(pointer,struct.pack('<Qii',data,count,2000))
                    source=self.r.read;calls={'names':0,'objects':0}
                    initial=source(getattr(self.r,prefix+'_header'),16)
                    data,num,capacity=struct.unpack('<Qii',initial)
                    observed=struct.pack('<Qii',data+(8 if change=='data' else 0),
                        num+(4 if change=='num' else 0),capacity+(1 if change=='max' else 0))
                    def read(address,size):
                        raw=source(address,size)
                        for name in calls:
                            if (address,size)==(getattr(self.r,name+'_header'),16):
                                calls[name]+=1
                                if name==prefix and calls[name]==2:return observed
                        return raw
                    self.r.read=read
                    with self.assertRaises(ForcedRegistryRefreshChanged) as raised:
                        self.reader.force_registry_refresh()
                    detail=raised.exception.read_conflict
                    self.assertEqual(str(raised.exception),'Registry header changed during forced refresh')
                    self.assertEqual(detail['stage'],'forced_registry_bracket')
                    self.assertEqual(detail['registry'],prefix)
                    self.assertEqual(detail['address'],hex(getattr(self.r,prefix+'_header')))
                    self.assertEqual(detail['initial_raw'],initial.hex())
                    self.assertEqual(detail['observed_raw'],observed.hex())
                    self.assertEqual(detail['initial'],dict(data=hex(data),num=num,max=capacity))
                    self.assertEqual(detail['observed'],dict(data=hex(data+(8 if change=='data' else 0)),
                        num=num+(4 if change=='num' else 0),max=capacity+(1 if change=='max' else 0)))
                    self.assertEqual(calls,dict(names=2,objects=1 if prefix=='names' else 2))

    def test_force_refresh_rejects_changed_or_invalid_headers(self):
        self.r.change_on_second_read=(self.r.names_header,16)
        with self.assertRaisesRegex(RuntimeError,'Registry header changed during forced refresh'):
            self.reader.sample()
        self.setUp()
        self.r.put(self.r.names_header,struct.pack('<Qii',self.r.names_data,1000,1001))
        with self.assertRaisesRegex(RuntimeError,'Invalid forced registry header'):
            self.reader.sample()

    def test_retry_still_rejects_changed_snapshot_values(self):
        original=self.reader.force_registry_refresh
        def refresh():
            original()
            self.r.read_counts.clear()
            offset=self.r.props['WarUIInteraction','BlockUIInputSemaphore']['offset']
            self.r.change_on_second_read=(0x4000+offset,4)
        with mock.patch.object(self.reader,'force_registry_refresh',side_effect=refresh) as forced:
            with self.assertRaisesRegex(RuntimeError,'UI changed before snapshot verification'):
                self.reader.sample()
            forced.assert_called_once()
        self.assertEqual(self.r.read.__func__,FakeResolver.read)

    def test_only_exact_runtime_error_gets_one_retry(self):
        class DifferentFailure(RuntimeError):pass
        for error in (DifferentFailure('Object registry identity changed'),
                      RuntimeError('Object registry identity changed: unknown'),
                      RuntimeError('Live object registry slot changed'),ValueError('Object registry identity changed')):
            with self.subTest(error=error),mock.patch.object(self.reader,'_sample_once',side_effect=error),\
                 mock.patch.object(self.reader,'force_registry_refresh') as forced:
                with self.assertRaises(type(error)):self.reader.sample()
                forced.assert_not_called()
        exact=RuntimeError('Object registry identity changed')
        with mock.patch.object(self.reader,'_sample_once',side_effect=exact) as sampled,\
             mock.patch.object(self.reader,'force_registry_refresh') as forced:
            with self.assertRaises(RuntimeError):self.reader.sample()
            self.assertEqual(sampled.call_count,2);forced.assert_called_once()


class DialogTextTests(unittest.TestCase):
    def setUp(self):
        MenuStateTests.setUp(self)
        self.r.base = 0x140000000
        pointer_struct = self.r.allocate()
        self.r.object(pointer_struct, 'Pointer', 'ScriptStruct', 0)
        self.r.struct_type = lambda prop: pointer_struct
        self.labels = {}
        self.add_label('Title', 'Online Service Error')
        self.add_label('MessageText', 'First line\n\nSecond line')
        self.r.array(0x1000, 'Children', [x['label'] for x in self.labels.values()])

    def add_label(self, name, text):
        r = self.r
        label, component, value, node = (r.allocate() for _ in range(4))
        r.object(label, name, 'UILabel', 0x1000)
        r.object(component, 'Render', 'UIComp_DrawString', label)
        r.object(value, 'Value', 'UIString', label)
        r.array(label, 'Children', [])
        r.scalar(label, 'StringRenderComponent', 'ComponentProperty', component, '<Q')
        r.scalar(component, 'ValueString', 'ObjectProperty', value, '<Q')
        # Later wrap nodes are opaque and deliberately unreadable in this fixture.
        data = r.array(value, 'Nodes', [node, 0xdeadbeef], 'StructProperty')
        r.put(node, struct.pack('<Q', r.base + 0x1348780))
        r.put(node + 0x10, struct.pack('<Q', 0))
        text_address = r.allocate()
        r.next_address += 0x1000
        raw = (text + '\0').encode('utf-16-le')
        r.put(text_address, raw)
        r.put(node + 0x18, struct.pack('<Qii', text_address, len(raw) // 2, len(raw) // 2))
        entry = dict(label=label, component=component, value=value, node=node,
                     text=text_address, data=data)
        self.labels[name] = entry
        return entry

    def read_dialog(self):
        return self.reader.dialog_text(self.r.identities[0x1000])

    def test_known_root_vtables_allow_later_wrap_nodes(self):
        for rva in (0x1348780, 0x13a4398):
            self.r.put(self.labels['Title']['node'], struct.pack('<Q', self.r.base + rva))
            self.assertEqual(self.read_dialog(), dict(Title='Online Service Error',
                                                      MessageText='First line\n\nSecond line'))

    def test_length_bounds_and_valid_maximum(self):
        entry = self.labels['Title']
        for pointer, length, capacity in ((0, 1, 1), (entry['text'], 0, 1),
                                          (entry['text'], -1, 1), (entry['text'], 2, 1),
                                          (entry['text'], 1, 1025)):
            with self.subTest(length=length, capacity=capacity):
                self.r.put(entry['node'] + 0x18, struct.pack('<Qii', pointer, length, capacity))
                with self.assertRaisesRegex(RuntimeError, 'Invalid dialog text bounds'):
                    self.read_dialog()
        self.r.put(entry['text'], ('x' * 1023 + '\0').encode('utf-16-le'))
        self.r.put(entry['node'] + 0x18, struct.pack('<Qii', entry['text'], 1024, 1024))
        self.assertEqual(len(self.read_dialog()['Title']), 1023)

    def test_missing_and_embedded_terminator(self):
        entry = self.labels['Title']
        for text in ('abc', 'a\0b\0'):
            raw = text.encode('utf-16-le')
            self.r.put(entry['text'], raw)
            self.r.put(entry['node'] + 0x18, struct.pack('<Qii', entry['text'], len(text), len(text)))
            with self.assertRaisesRegex(RuntimeError, 'Invalid dialog text terminator'):
                self.read_dialog()

    def test_foreign_component_and_uistring_owner(self):
        entry = self.labels['Title']
        for field, error in (('component', 'Dialog string component mismatch'),
                             ('value', 'Dialog UIString owner mismatch')):
            original = self.r.identities[entry[field]]['outer']
            self.r.identities[entry[field]]['outer'] = 0x3000
            with self.assertRaisesRegex(RuntimeError, error):
                self.read_dialog()
            self.r.identities[entry[field]]['outer'] = original

    def test_unknown_vtable_and_nonroot_parent(self):
        entry = self.labels['Title']
        self.r.put(entry['node'], struct.pack('<Q', self.r.base + 0x123456))
        with self.assertRaisesRegex(RuntimeError, 'Unsupported dialog node vtable/parent'):
            self.read_dialog()
        self.r.put(entry['node'], struct.pack('<Q', self.r.base + 0x1348780))
        self.r.put(entry['node'] + 0x10, struct.pack('<Q', 1))
        with self.assertRaisesRegex(RuntimeError, 'Unsupported dialog node vtable/parent'):
            self.read_dialog()

    def test_duplicate_label_rejected(self):
        original = self.labels['Title']['label']
        duplicate = self.add_label('Title', 'Other')
        self.r.array(0x1000, 'Children', [original, duplicate['label'], self.labels['MessageText']['label']])
        with self.assertRaisesRegex(RuntimeError, 'Ambiguous dialog label'):
            self.read_dialog()

    def test_missing_label_and_child_owner_rejected(self):
        self.r.array(0x1000, 'Children', [self.labels['Title']['label']])
        with self.assertRaisesRegex(RuntimeError, 'Missing dialog labels'):
            self.read_dialog()
        self.r.identities[self.labels['Title']['label']]['outer'] = 0x3000
        with self.assertRaisesRegex(RuntimeError, 'Dialog child owner mismatch'):
            self.read_dialog()

    def source_address(self, name='Title'):
        value = self.labels[name]['value']
        return value + self.r.props['UIString', 'Nodes']['offset']

    def replace_native_root(self, name, text):
        entry = self.labels[name]
        node, text_address = self.r.allocate(), self.r.allocate()
        raw = (text + '\0').encode('utf-16-le')
        self.r.put(node, struct.pack('<Q', self.r.base + 0x13a4398))
        self.r.put(node + 0x10, bytes(8))
        self.r.put(node + 0x18, struct.pack('<Qii', text_address, len(raw)//2, len(raw)//2))
        self.r.put(text_address, raw)
        self.r.array(entry['value'], 'Nodes', [node, 0xdeadbeef], 'StructProperty')
        return node

    def use_confirmation_sample(self):
        self.r.identities[0x1000]['cls'] = 'UIScene_Confirmation'
        for (owner, name), prop in list(self.r.props.items()):
            if owner == 'Scene':
                self.r.props['UIScene_Confirmation', name] = prop
        self.r.name = lambda index: 'WarfareConfirmation'
        self.r.array(0x1000, 'FocusControls', [], 'StructProperty')

    def change_title_after_initial_acquisition(self, text):
        original = self.r.read
        triggered = []
        def changing(address, size):
            raw = original(address, size)
            if (address, size) == (self.source_address(), 16):
                triggered.append(True)
            if len(triggered) == 3 and (address, size) == (self.source_address(), 16):
                self.replace_native_root('Title', text)
            return raw
        self.r.read = changing

    def test_sample_allows_reallocated_nodes_root_same_text(self):
        self.use_confirmation_sample()
        self.change_title_after_initial_acquisition('Online Service Error')
        result = self.reader.sample()
        self.assertEqual(result['dialog_text']['Title'], 'Online Service Error')
        self.assertEqual(result['dialog_text_consistency'], 'bounded_root_acquisition_and_semantic_reread')
        self.assertTrue(result['consistent_reread'])

    def test_sample_rejects_reallocated_root_changed_text(self):
        self.use_confirmation_sample()
        self.change_title_after_initial_acquisition('Load checkpoint instead')
        with self.assertRaisesRegex(RuntimeError, 'Dialog source text changed'):
            self.reader.sample()

    def test_short_bracket_retries_once_after_array_reallocation(self):
        address = self.source_address()
        original = self.r.read
        calls = []
        def changing(at, size):
            if (at, size) == (address, 16):
                calls.append(True)
                if len(calls) == 2:
                    self.replace_native_root('Title', 'Online Service Error')
            return original(at, size)
        self.assertEqual(read_root_source_text(changing, address, self.r.base), 'Online Service Error')
        self.assertEqual(len(calls), 4)

    def test_persistent_storage_churn_is_bounded_and_rejected(self):
        address = self.source_address()
        original = self.r.read
        calls = []
        def changing(at, size):
            if (at, size) == (address, 16):
                calls.append(True)
                self.replace_native_root('Title', 'Online Service Error')
            return original(at, size)
        with self.assertRaises(DialogStorageChanged):
            read_root_source_text(changing, address, self.r.base)
        self.assertEqual(len(calls), 6)

    def test_nodes_bounds_and_pointer_alignment_rejected(self):
        address = self.source_address()
        for pointer, count, capacity in [(0,1,1),(3,1,1),(8,0,1),(8,2,1),(8,1,65)]:
            with self.subTest(pointer=pointer, count=count):
                self.r.put(address, struct.pack('<Qii', pointer,count,capacity))
                with self.assertRaisesRegex(RuntimeError, 'Invalid bounded array: Nodes'):
                    read_root_source_text(self.r.read,address,self.r.base)

    def test_null_and_unaligned_root_rejected(self):
        entry = self.labels['Title']
        for node in (0,3):
            self.r.put(entry['data'], struct.pack('<Q',node))
            with self.assertRaisesRegex(RuntimeError, 'Invalid dialog root pointer'):
                read_root_source_text(self.r.read,self.source_address(),self.r.base)

    def test_final_source_reread_still_rejects_bad_vtable(self):
        self.use_confirmation_sample()
        original = self.r.read
        triggered = []
        def changing(address,size):
            raw = original(address,size)
            if not triggered and (address,size)==(self.labels['MessageText']['node']+0x18,16):
                triggered.append(True)
                self.r.put(self.labels['Title']['node'],struct.pack('<Q',self.r.base+0x123456))
            return raw
        self.r.read=changing
        with self.assertRaisesRegex(RuntimeError,'Unsupported dialog node vtable/parent'):
            self.reader.sample()

    def test_utf16_content_change_during_acquisition_not_accepted_unchecked(self):
        entry=self.labels['Title'];original=self.r.read;calls=[]
        # Keep the original length and change one code unit during the bracket.
        def changing(address,size):
            data=original(address,size)
            if address==entry['text']:
                calls.append(True)
                if len(calls)==1:self.r.put(entry['text'],b'X\0'+data[2:])
            return data
        self.assertEqual(read_root_source_text(changing,self.source_address(),self.r.base),'Xnline Service Error')
        self.assertGreaterEqual(len(calls),4)

    def test_wellformed_empty_rebuild_retries_until_valid(self):
        address=self.source_address();original=self.r.read;calls=[]
        def empty_once(at,size):
            if (at,size)==(address,16):
                calls.append(True)
                if len(calls)==1:return struct.pack('<Qii',0,0,0)
            return original(at,size)
        self.assertEqual(read_root_source_text(empty_once,address,self.r.base),'Online Service Error')
        self.assertEqual(len(calls),3)

    def test_persistent_wellformed_empty_is_not_text_and_is_bounded(self):
        address=self.source_address();original=self.r.read;calls=[]
        def always_empty(at,size):
            if (at,size)==(address,16):
                calls.append(True);return struct.pack('<Qii',0,0,0)
            return original(at,size)
        with mock.patch('menu_state.time.sleep') as slept,self.assertRaises(DialogStorageChanged):
            read_root_source_text(always_empty,address,self.r.base)
        self.assertEqual(len(calls),3)
        self.assertEqual(slept.call_args_list,[mock.call(.001),mock.call(.001)])

    def test_invalid_string_header_rechecks_each_storage_link_before_retry(self):
        for source in ('nodes','root','vtable','parent','string'):
            with self.subTest(source=source):
                self.setUp();entry=self.labels['Title'];original=self.r.read;triggered=[]
                target={'nodes':(self.source_address(),16),'root':(entry['data'],8),
                        'vtable':(entry['node'],8),'parent':(entry['node']+0x10,8),
                        'string':(entry['node']+0x18,16)}[source]
                saved=original(*target)
                def changing(at,size):
                    raw=original(at,size)
                    if (at,size)==(entry['node']+0x18,16) and not triggered:
                        triggered.append(True)
                        if source!='string':self.r.put(target[0],bytes([saved[0]^8])+saved[1:])
                        return struct.pack('<Qii',0,0,0)
                    return raw
                def backoff(seconds):self.r.put(target[0],saved)
                with mock.patch('menu_state.time.sleep',side_effect=backoff) as slept:
                    self.assertEqual(read_root_source_text(changing,self.source_address(),self.r.base),'Online Service Error')
                    slept.assert_called_once_with(.001)

    def test_invalid_root_vtable_parent_retry_only_after_a_changed_bracket(self):
        for field in ('null_root','unaligned_root','vtable','parent'):
            with self.subTest(field=field):
                self.setUp();entry=self.labels['Title'];original=self.r.read;triggered=[]
                target=(entry['data'],8) if field.endswith('root') else (entry['node']+(0x10 if field=='parent' else 0),8)
                bad=struct.pack('<Q',{'null_root':0,'unaligned_root':3,'vtable':self.r.base+0x123456,'parent':8}[field])
                def transient(at,size):
                    if (at,size)==target and not triggered:
                        triggered.append(True);return bad
                    return original(at,size)
                with mock.patch('menu_state.time.sleep') as slept:
                    self.assertEqual(read_root_source_text(transient,self.source_address(),self.r.base),'Online Service Error')
                    slept.assert_called_once_with(.001)
                self.r.put(target[0],bad)
                with mock.patch('menu_state.time.sleep') as slept,self.assertRaises(RuntimeError) as rejected:
                    read_root_source_text(original,self.source_address(),self.r.base)
                self.assertIs(type(rejected.exception),RuntimeError);slept.assert_not_called()

    def test_stable_invalid_string_bounds_never_retry_or_sleep(self):
        entry=self.labels['Title'];self.r.put(entry['node']+0x18,struct.pack('<Qii',0,0,0))
        with mock.patch('menu_state.time.sleep') as slept,self.assertRaisesRegex(RuntimeError,'Invalid dialog text bounds') as error:
            read_root_source_text(self.r.read,self.source_address(),self.r.base)
        self.assertIs(type(error.exception),RuntimeError);slept.assert_not_called()
        self.assertEqual(self.r.read_counts[(self.source_address(),16)],2)
        self.assertEqual(self.r.read_counts[(entry['node']+0x18,16)],2)

    def test_normal_source_acquisition_has_no_backoff(self):
        with mock.patch('menu_state.time.sleep') as slept:
            self.assertEqual(read_root_source_text(self.r.read,self.source_address(),self.r.base),'Online Service Error')
            slept.assert_not_called()

    def test_dialog_storage_race_shares_outer_snapshot_retry_budget(self):
        self.use_confirmation_sample();original=self.reader._sample_once
        # First full acquisition exhausts its inner three attempts, then the
        # second starts afresh and succeeds. No resolver or inventory rebuild.
        address=self.source_address();read=self.r.read;empty=[0]
        def temporarily_empty(at,size):
            if (at,size)==(address,16) and empty[0]<3:
                empty[0]+=1;return struct.pack('<Qii',0,0,0)
            return read(at,size)
        self.r.read=temporarily_empty
        with mock.patch.object(self.reader,'_sample_once',wraps=original) as sampled,\
             mock.patch.object(self.reader,'force_registry_refresh') as forced,mock.patch('menu_state.time.sleep') as slept:
            result=self.reader.sample()
            self.assertEqual(sampled.call_count,2);forced.assert_not_called()
            self.assertEqual(slept.call_args_list,[mock.call(.001),mock.call(.001)])
        self.assertEqual(result['dialog_text']['Title'],'Online Service Error')
        for errors in ((DialogStorageChanged('empty'),SnapshotChanged('changed')),
                       (SnapshotChanged('changed'),DialogStorageChanged('empty')),
                       (DialogStorageChanged('empty'),RuntimeError('Object registry identity changed')),
                       (RuntimeError('Object registry identity changed'),DialogStorageChanged('empty')),
                       (DialogStorageChanged('empty'),DialogStorageChanged('empty'))):
            with mock.patch.object(self.reader,'_sample_once',side_effect=errors) as sampled,\
                 mock.patch.object(self.reader,'force_registry_refresh') as forced:
                with self.assertRaises(type(errors[-1])):self.reader.sample()
                self.assertEqual(sampled.call_count,2)
                self.assertEqual(forced.call_count,int(type(errors[0]) is RuntimeError))


if __name__ == '__main__':
    unittest.main()
