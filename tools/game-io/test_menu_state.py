"""Offline menu reader tests; no process handles or input are created."""
import struct
import unittest
from unittest import mock

from menu_state import MenuReader, registered, read_root_source_text, DialogStorageChanged, SnapshotChanged, InventoryChanged


class FakeResolver:
    def __init__(self):
        self.pid = 123
        self.profile = {'struct_property_struct': 208}
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
        return obj['cls']

    def property(self, owner, name, inherited=False):
        if owner == self.focus_struct:
            return dict(address=0, offset={'FocusedControl': 0, 'LastFocusedControl': 8}[name],
                        type='ObjectProperty')
        return self.props[owner, name]

    def scalar(self, address, name, kind, value, fmt):
        owner = self.identities[address]['cls']
        key = owner, name
        if key not in self.props:
            offset = 0x100 + sum(k[0] == owner for k in self.props) * 0x20
            self.props[key] = dict(address=self.allocate(), offset=offset, type=kind)
        values = value if isinstance(value, tuple) else (value,)
        self.put(address + self.props[key]['offset'], struct.pack(fmt, *values))

    def array(self, address, name, values, kind='ObjectProperty'):
        owner = self.identities[address]['cls']
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


class MenuStateTests(unittest.TestCase):
    def setUp(self):
        self.r = FakeResolver()
        self.reader = MenuReader.__new__(MenuReader)
        self.reader.r, self.reader.client, self.reader.properties = self.r, 0x3000, {}
        # Focus on actual sample validation; no OS APIs or registry enumeration.
        self.reader.refresh = lambda: None

    def add_tree_widget(self,address,parent,name='Branch'):
        self.r.object(address,name,'Widget',parent)
        self.r.array(address,'Children',[])
        self.r.array(address,'StateStack',[])
        self.r.array(address,'FocusControls',[],'StructProperty')

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
