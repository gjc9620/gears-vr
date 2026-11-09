"""Full reachable-tree fixtures with real CoverQuery; never runs the game."""
import copy
import json
from pathlib import Path
import struct
import unittest
from unittest.mock import patch

import cover_spatial_inventory as inventory
import cover_spatial_root as root
from cover_state import CoverQuery
from test_cover_spatial_root import SpatialMemory


class InventoryMemory(SpatialMemory):
    PRIMITIVE_CLASS, SUB_CLASS = 0x500000, 0x501000
    FIRST_COMPONENT = 0x510000

    def __init__(self, wrapper_code, tree_code):
        super().__init__(wrapper_code, tree_code)
        self.add_class(self.PRIMITIVE_CLASS, 'PrimitiveComponent', 0)
        self.add_class(self.SUB_CLASS, 'StaticMeshComponent', self.PRIMITIVE_CLASS)
        self.next_storage = 0x600000
        self.forward = {}

    def storage(self, data):
        address = self.next_storage;self.next_storage += 0x1000
        self.put(address, data)
        return address

    def split(self, address, *, start=None):
        start = self.next_storage + 8 if start is None else start
        if start == self.next_storage + 8:self.next_storage += 0x1000
        self.put(address + 16, struct.pack('<Q', start))
        self.put(start - 8, struct.pack('<Q', 8))
        self.put(start, bytes(8 * 24))
        return [start + i * 24 for i in range(8)]

    def members(self, node, addresses):
        data = self.storage(struct.pack('<' + str(len(addresses)) + 'Q', *addresses)) if addresses else 0
        self.put(node, struct.pack('<Qii', data, len(addresses), len(addresses)))
        self.forward[node] = list(addresses)
        return data

    def finish_components(self):
        reverse = {}
        for node, members in self.forward.items():
            for component in members:reverse.setdefault(component, []).append(node)
        self.reverse_payload = {}
        for address, nodes in reverse.items():
            self.add_object(address, 'TreeComponent', self.SUB_CLASS)
            data = self.storage(struct.pack('<' + str(len(nodes)) + 'Q', *nodes))
            self.put(address + inventory.REVERSE_MEMBERS_OFFSET, struct.pack('<Qii', data, len(nodes), len(nodes)))
            self.reverse_payload[address] = data


class SpatialInventoryTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if not Path(root.surface.DEFAULT_BLOB).is_file():
            raise unittest.SkipTest('Retained original code required for inventory regression')
        cls.wrapper_code=root.world_code._historical();cls.tree_code=root._historical()

    def fixture(self):
        f=InventoryMemory(self.wrapper_code,self.tree_code)
        return f,CoverQuery(f.observer)

    def observe(self,f,q):
        with patch.object(root.world_code,'_historical',return_value=self.wrapper_code), \
                patch.object(root,'_historical',return_value=self.tree_code):
            return inventory.observe(q,f.module,copy.deepcopy(f.objects[f.WORLD]))

    def populated(self):
        f,q=self.fixture()
        children=f.split(f.NODE)
        grandchildren=f.split(children[5])
        a,b,c=[f.FIRST_COMPONENT+0x1000*i for i in range(3)]
        f.members(f.NODE,[a])
        f.members(children[0],[b])
        f.members(children[7],[a,b])
        f.members(grandchildren[7],[c])
        f.finish_components()
        return f,q,children,grandchildren,(a,b,c)

    def test_every_reachable_node_including_eighth_grandchild_and_shared_components(self):
        f,q,children,grandchildren,addresses=self.populated();before=copy.deepcopy(f.memory)
        out=self.observe(f,q)
        self.assertEqual(out['node_count'],17)
        self.assertEqual(out['component_count'],3)
        self.assertEqual(out['component_slots'],5)
        self.assertEqual({n['address'] for n in out['nodes']},{f.NODE,*children,*grandchildren})
        deepest=next(n for n in out['nodes'] if n['address']==grandchildren[7])
        self.assertEqual((deepest['parent'],deepest['child_index'],deepest['depth']),(children[5],7,2))
        records={r['identity']['address']:r for r in out['components']}
        self.assertEqual(records[addresses[0]]['node_addresses'],[f.NODE,children[7]])
        self.assertEqual(records[addresses[1]]['node_addresses'],[children[0],children[7]])
        self.assertEqual(records[addresses[2]]['node_addresses'],[grandchildren[7]])
        for component in addresses:
            self.assertIn((component+0x164,16),q.values)
            self.assertNotIn((component+0x168,16),q.values)
        self.assertTrue(out['all_reachable_tree_records_observed'] and out['forward_reverse_membership_observed'])
        self.assertFalse(out['empty_reachable_tree'])
        for key in ('native_stamp_written','engine_calls','virtual_calls','valid_empty_collision_result',
                    'overflow_or_other_hash_storage_observed','bsp_observed','owner_world_association_verified',
                    'shape_or_filter_coverage_verified','complete_scene_verified','atomic_snapshot_verified',
                    'world_synchronization_verified','path_or_occupancy_verified','activation_allowed'):
            self.assertFalse(out[key],key)
        q.finish();self.assertEqual(f.memory,before);self.assertEqual(f.bridges,[])
        json.dumps(out,allow_nan=False)

    def test_empty_tree_and_allocated_empty_children_do_not_certify_empty_collision_world(self):
        for split in (False,True):
            f,q=self.fixture()
            if split:f.split(f.NODE)
            out=self.observe(f,q)
            self.assertEqual((out['component_count'],out['component_slots']),(0,0))
            self.assertEqual(out['node_count'],9 if split else 1)
            self.assertTrue(out['empty_reachable_tree'])
            self.assertFalse(out['valid_empty_collision_result'])
            self.assertFalse(out['complete_scene_verified'])
            q.finish()

    def test_duplicate_in_one_node_rejects_but_cross_node_is_legitimate(self):
        f,q=self.fixture();a=f.FIRST_COMPONENT
        f.members(f.NODE,[a,a]);f.finish_components()
        with self.assertRaisesRegex(RuntimeError,'duplicate within node'):self.observe(f,q)

    def test_bad_component_class_default_and_class_cycle_reject(self):
        for defect in ('class','default','class_cycle','class_number','class_meta'):
            with self.subTest(defect=defect):
                f,q,*_=self.populated();a=f.FIRST_COMPONENT
                if defect=='class':f.objects[a]['cls']=f.WORLD_CLASS
                elif defect=='default':f.objects[a]['name']='Default__TreeComponent'
                elif defect=='class_cycle':f.put(f.PRIMITIVE_CLASS+f.profile['field_super'],struct.pack('<Q',f.SUB_CLASS))
                elif defect=='class_number':f.objects[f.SUB_CLASS]['number']=1
                else:f.objects[f.SUB_CLASS]['cls']=f.WORLD_CLASS
                with self.assertRaises(RuntimeError):self.observe(f,q)

    def test_null_unaligned_and_out_of_range_forward_entries_reject(self):
        for pointer in (0,0x10001,1<<47,(1<<64)-8):
            f,q=self.fixture();data=f.members(f.NODE,[pointer])
            with self.assertRaisesRegex(RuntimeError,'pointer'):self.observe(f,q)
            self.assertIn((data,8),q.values)

    def test_all_reverse_members_must_match_all_forward_nodes(self):
        for defect in ('empty','missing_one','unknown_node','duplicate','unaligned_node','null_node'):
            with self.subTest(defect=defect):
                f,q,children,_,addresses=self.populated();a=addresses[0]
                values={'empty':[],'missing_one':[f.NODE],'unknown_node':[f.NODE,0x700000],
                        'duplicate':[f.NODE,f.NODE],'unaligned_node':[f.NODE,0x10001],
                        'null_node':[f.NODE,0]}[defect]
                data=f.storage(struct.pack('<'+str(len(values))+'Q',*values)) if values else 0
                f.put(a+0x164,struct.pack('<Qii',data,len(values),len(values)))
                with self.assertRaises(RuntimeError):self.observe(f,q)

    def test_reverse_header_capacity_and_storage_reject(self):
        for data,count,capacity in ((0,-1,0),(0,1,0),(0,0,-1),(0,1,1),
                (0,0,root.MAX_NODE_CAPACITY+1),(0x700000,0,0),(0x700001,0,1)):
            f,q,*_=self.populated()
            f.put(f.FIRST_COMPONENT+0x164,struct.pack('<Qii',data,count,capacity))
            with self.assertRaises(RuntimeError):self.observe(f,q)

    def test_child_allocation_alias_and_deeper_cycle_reject(self):
        for defect in ('shared_allocation','back_to_root','overlapping_allocation'):
            with self.subTest(defect=defect):
                f,q,children,grandchildren,_=self.populated()
                if defect=='shared_allocation':f.put(children[6]+16,struct.pack('<Q',grandchildren[0]))
                elif defect=='back_to_root':f.put(grandchildren[0]+16,struct.pack('<Q',children[0]))
                else:f.put(children[6]+16,struct.pack('<Q',grandchildren[0]+8))
                with self.assertRaisesRegex(RuntimeError,'alias|cycle|overlap'):self.observe(f,q)

    def test_deeper_cookie_and_header_failures_do_not_return_partial_success(self):
        for defect in ('cookie','count','capacity','storage'):
            f,q,_,grandchildren,_=self.populated()
            if defect=='cookie':f.put(grandchildren[0]-8,struct.pack('<Q',7))
            elif defect=='count':f.put(grandchildren[7]+8,struct.pack('<i',-1))
            elif defect=='capacity':f.put(grandchildren[7]+12,struct.pack('<i',0))
            else:f.put(grandchildren[7],struct.pack('<Q',0))
            with self.assertRaises(RuntimeError):self.observe(f,q)

    def test_observation_limits_reject_instead_of_truncating(self):
        for name,value in (('MAX_NODES',16),('MAX_DEPTH',1),('MAX_COMPONENT_SLOTS',4),
                           ('MAX_COMPONENTS',2),('MAX_REVERSE_MEMBERS',1)):
            with self.subTest(name=name):
                f,q,*_=self.populated()
                with patch.object(inventory,name,value),self.assertRaisesRegex(RuntimeError,'bound'):self.observe(f,q)

    def test_component_slot_bound_precedes_payload_read(self):
        f,q=self.fixture();data=f.members(f.NODE,[f.FIRST_COMPONENT+i*0x1000 for i in range(4)])
        with patch.object(inventory,'MAX_COMPONENT_SLOTS',3),self.assertRaisesRegex(RuntimeError,'slot bound'):self.observe(f,q)
        self.assertNotIn((data,32),q.values)

    def test_change_of_node_after_group_witness_rejects_during_walk(self):
        f,q,children,grandchildren,_=self.populated()
        def mutate(address,size,count):
            if address==grandchildren[7] and size==24:
                f.put(address+8,struct.pack('<i',0))
        f.before_read=mutate
        with self.assertRaisesRegex(RuntimeError,'changed'):self.observe(f,q)

    def test_every_new_inventory_source_change_rejects_same_query_close(self):
        for defect in ('forward','reverse_header','reverse_payload','grandchild_header','grandchild_cookie','component_identity'):
            with self.subTest(defect=defect):
                f,q,_,grandchildren,addresses=self.populated();self.observe(f,q)
                if defect=='component_identity':f.objects[addresses[2]]['internal_index']+=1
                else:
                    node_data=struct.unpack('<Q',f.read(f.NODE,8))[0]
                    address={'forward':node_data,'reverse_header':addresses[0]+0x164,
                        'reverse_payload':f.reverse_payload[addresses[0]],
                        'grandchild_header':grandchildren[7]+8,'grandchild_cookie':grandchildren[0]-8}[defect]
                    f.put(address,bytes([f.memory[address]^1]))
                with self.assertRaisesRegex(RuntimeError,'changed'):q.finish()

    def test_existing_query_budget_remains_in_force(self):
        f,q=self.fixture();q.reads=16384
        with self.assertRaisesRegex(RuntimeError,'budget'):self.observe(f,q)


if __name__=='__main__':
    unittest.main()
