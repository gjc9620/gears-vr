"""Death/checkpoint preparation with real UI certificates over mocked memory."""
import contextlib
import io
import pathlib
import struct
import unittest
from unittest import mock
import start_capture as start
import anchor_config as anchor
import ui_observer_config as ui
from test_ui_observer_config import Fixture


class PawnPreparationTests(unittest.TestCase):
    def test_only_fresh_validated_null_controller_pawn_is_typed(self):
        r=Fixture();r.put(r.pc+0x224,bytes(8))
        with self.assertRaises(ui.UIControlledPawnUnavailable) as raised:
            ui.build_ui_observer_config(r,r.camera)
        evidence=raised.exception.evidence
        self.assertEqual((evidence['controller']['address'],evidence['field']['name'],
                          evidence['offset'],evidence['pointer']),(r.pc,'Pawn',0x224,0))
        self.assertTrue(start.camera_preparation_transient(raised.exception))
        for mutation in ('type','slot','offset','other_null'):
            with self.subTest(mutation=mutation):
                r=Fixture();r.put(r.pc+0x224,bytes(8))
                field=r.props[r.classes['PlayerController'],'Pawn']['address']
                if mutation=='type':r.put(field+80,struct.pack('<Q',r.classes['IntProperty']))
                if mutation=='slot':r.put(r.od+r.objects.index(r.pc)*8,bytes(8))
                if mutation=='offset':r.put(field+148,struct.pack('<I',0x228))
                if mutation=='other_null':
                    r.put(r.pc+0x224,struct.pack('<Q',r.pawn));r.put(r.pc+0x41c,bytes(8))
                with self.assertRaises(ui.UIConfigRejected) as rejected:
                    ui.build_ui_observer_config(r,r.camera)
                self.assertFalse(start.camera_preparation_transient(rejected.exception))
        class UnknownPawnFailure(ui.UIControlledPawnUnavailable):pass
        for error in (ui.UIConfigRejected('UI reference mismatch: controller_pawn'),
                      RuntimeError('UI controlled Pawn unavailable; reacquire'),
                      UnknownPawnFailure({}, {}, 0x224)):
            self.assertFalse(start.camera_preparation_transient(error))

    def test_null_classification_rechecks_source_field_and_offset(self):
        for mutation in ('controller','field','offset'):
            with self.subTest(mutation=mutation):
                r=Fixture();original=r.u64;r.put(r.pc+0x224,bytes(8));changed=False
                field=r.props[r.classes['PlayerController'],'Pawn']['address']
                def read(address):
                    nonlocal changed
                    value=original(address)
                    if address==r.pc+0x224 and not changed:
                        changed=True
                        if mutation=='offset':r.put(field+148,struct.pack('<I',0x228))
                        else:r.put((r.pc if mutation=='controller' else field)+76,struct.pack('<I',1))
                    return value
                r.u64=read
                with self.assertRaises(ui.UIConfigRejected) as rejected:
                    ui.build_ui_observer_config(r,r.camera)
                self.assertFalse(start.camera_preparation_transient(rejected.exception))

    def test_death_after_anchor_waits_for_checkpoint_or_shared_deadline_cancel(self):
        for outcome in ('checkpoint','deadline','cancel','code'):
            with self.subTest(outcome=outcome),contextlib.ExitStack() as stack:
                resolvers=[];ticks=[0.];anchors=[]
                baseline=dict(local_player_address='0x100',module_base='0x140000000')
                def resolver(pid,profile):
                    r=Fixture();r.generation=len(resolvers)+1
                    if r.generation>4:
                        r.pawn=r.obj('Pawn_Checkpoint',r.classes['Pawn_COGMarcus'])
                        for address,target in ((r.pc+0x224,r.pawn),(r.pawn+0x230,r.pc),(r.pawn+0x434,r.inventory)):
                            r.put(address,struct.pack('<Q',target))
                        r.finish_registry()
                    r.camera.update(baseline,generation=r.generation)
                    r.resolve=mock.Mock(return_value=r.camera);r.p=mock.Mock()
                    resolvers.append(r);return r
                def build_anchor(r,c,*,prepared_camera=None):
                    self.assertIs(prepared_camera,c)
                    anchors.append((r,c))
                    result=dict(generation=r.generation,pawn=r.pawn)
                    # Valid anchor acquired first; death happens before UI reads.
                    if outcome!='checkpoint' or r.generation<=4:r.put(r.pc+0x224,bytes(8))
                    return result,result.copy()
                def check():
                    if outcome=='cancel' and len(resolvers)==3:raise RuntimeError('Owner cancelled')
                def pause(duration):ticks[0]+=duration
                stack.enter_context(mock.patch.object(start,'Resolver',side_effect=resolver))
                stack.enter_context(mock.patch.object(anchor,'build_anchor_config',side_effect=build_anchor))
                stack.enter_context(mock.patch.object(anchor,'anchor_ini',return_value='anchor\n'))
                stack.enter_context(mock.patch.object(start.time,'monotonic',side_effect=lambda:ticks[0]))
                stack.enter_context(mock.patch.object(start.time,'sleep',side_effect=pause))
                remote=stack.enter_context(mock.patch.object(start,'remote_call'))
                written=stack.enter_context(mock.patch.object(pathlib.Path,'write_text'))
                stack.enter_context(contextlib.redirect_stdout(io.StringIO()))
                stack.enter_context(contextlib.redirect_stderr(io.StringIO()))
                deadline=.7 if outcome=='deadline' else 5
                def read_entry(address,size,original):
                    if address==0x140000001:
                        return b'wrong' if outcome=='code' and len(resolvers)>3 else b'bytes'
                    return original(address,size)
                def prepare():
                    return start.wait_camera_preparation(lambda:start.acquire_observer_preparation(
                        123,{},baseline,{1:b'bytes'},pathlib.Path('Unused.dll'),anchor_observer=True,ui_observer=True,
                        preparation_deadline=deadline,check_live=check,entry_reader=read_entry),
                        check,deadline,clock=lambda:ticks[0],sleep=pause)
                if outcome=='checkpoint':
                    camera,am,asection,um,usection=prepare()
                    config=um['config'];pawn=config['identities'][config['roles']['pawn']]
                    self.assertEqual((camera['generation'],am['generation'],pawn['address']),
                                     (5,5,resolvers[-1].pawn))
                    self.assertEqual(pawn['name'],'Pawn_Checkpoint')
                    self.assertEqual(len(anchors),5)
                    self.assertIn('[UIObserver]',usection)
                else:
                    with self.assertRaisesRegex(RuntimeError if outcome!='deadline' else (TimeoutError,ui.UIConfigRejected),
                                                'cancelled' if outcome=='cancel' else 'prologue' if outcome=='code' else 'deadline'):
                        prepare()
                self.assertEqual(len(resolvers),5 if outcome=='checkpoint' else 3 if outcome=='cancel' else 4 if outcome=='code' else 5)
                self.assertLessEqual(ticks[0],deadline)
                for r in resolvers:r.p.close.assert_called_once();r.resolve.assert_called_once()
                remote.assert_not_called();written.assert_not_called()


if __name__=='__main__':unittest.main()
