"""Initial startup discovery consolidation; all process/native operations mocked."""
import contextlib
import io
import pathlib
import unittest
from unittest import mock

import start_capture as start
import anchor_config as anchor
import ui_observer_config as ui


class InitialPreparationTests(unittest.TestCase):
    def test_first_successful_reference_survives_both_retry_layers(self):
        for outcome in ('success','local_player','module'):
            with self.subTest(outcome=outcome),contextlib.ExitStack() as stack:
                state=start._InitialPreparationCamera();resolvers=[];built=[];ticks=[0.]
                baseline=dict(local_player_address='0x1',module_base='0x1000')
                def resolver(pid,profile):
                    r=mock.Mock();r.base=0x1000;r.read.return_value=b'ok'
                    generation=len(resolvers)+1
                    camera=dict(baseline,camera_address=f'camera{generation}')
                    if generation==5 and outcome!='success':
                        camera['local_player_address' if outcome=='local_player' else 'module_base']='0x2000'
                    r.resolve.return_value=camera
                    # A failed discovery must not establish the reference.
                    if generation==1:r.resolve.side_effect=RuntimeError('Expected one local camera, found 0; no address accepted')
                    resolvers.append(r);return r
                def build_anchor(r,camera,*,prepared_camera):
                    self.assertIs(camera,prepared_camera);self.assertIs(camera,r.resolve.return_value)
                    self.assertEqual(state.reference,baseline)
                    built.append(camera)
                    return {'camera':camera['camera_address']},{'camera':camera['camera_address']}
                def build_ui(r,camera):
                    self.assertIs(camera,built[-1])
                    if len(resolvers)<5:raise ui.UIRegistryChanged('Registry changed')
                    return {'camera':camera['camera_address']},{'camera':camera['camera_address']}
                def pause(duration):ticks[0]+=duration
                stack.enter_context(mock.patch.object(start,'Resolver',side_effect=resolver))
                stack.enter_context(mock.patch.object(anchor,'build_anchor_config',side_effect=build_anchor))
                stack.enter_context(mock.patch.object(anchor,'anchor_ini',side_effect=lambda c:'anchor:'+c['camera']))
                stack.enter_context(mock.patch.object(ui,'build_ui_observer_config',side_effect=build_ui))
                stack.enter_context(mock.patch.object(ui,'ui_observer_ini',side_effect=lambda c:'ui:'+c['camera']))
                stack.enter_context(mock.patch.object(start.time,'monotonic',side_effect=lambda:ticks[0]))
                stack.enter_context(mock.patch.object(start.time,'sleep',side_effect=pause))
                remote=stack.enter_context(mock.patch.object(start,'remote_call'))
                write=stack.enter_context(mock.patch.object(pathlib.Path,'write_text'))
                stack.enter_context(contextlib.redirect_stdout(io.StringIO()))
                stack.enter_context(contextlib.redirect_stderr(io.StringIO()))
                def acquire():
                    return start.acquire_observer_preparation(1,{},state,{1:b'ok'},pathlib.Path('Unused.dll'),
                        anchor_observer=True,ui_observer=True,record_anchor_failure=False,preparation_deadline=10)
                def wait():
                    return start.wait_camera_preparation(acquire,lambda:None,10,clock=lambda:ticks[0],sleep=pause)
                if outcome=='success':
                    result=wait()
                    self.assertEqual((result[0]['camera_address'],result[1]['camera'],result[3]['camera']),('camera5',)*3)
                    self.assertEqual((result[2],result[4]),('anchor:camera5','ui:camera5'))
                else:
                    with self.assertRaisesRegex(RuntimeError,'Local player changed' if outcome=='local_player' else 'Module base changed'):
                        wait()
                self.assertEqual(state.reference,baseline);self.assertEqual(len(resolvers),5)
                self.assertEqual(len(built),4 if outcome=='success' else 3)
                for r in resolvers:r.resolve.assert_called_once();r.p.close.assert_called_once()
                remote.assert_not_called();write.assert_not_called()

    def test_initial_cancellation_deadline_and_late_success_prevent_activation(self):
        for outcome in ('cancel','deadline','late_success'):
            with self.subTest(outcome=outcome),contextlib.ExitStack() as stack:
                state=start._InitialPreparationCamera();ticks=[0.];calls=[0]
                camera=dict(local_player_address='0x1',module_base='0x1000')
                r=mock.Mock();r.resolve.return_value=camera;r.base=0x1000;r.read.return_value=b'ok'
                def build(r,camera,**kwargs):
                    if outcome=='late_success':ticks[0]=2;return {},{}
                    raise anchor.AnchorSamplingRejected('Camera view target is not controlled Pawn',{'stage':'initial'})
                def check():
                    calls[0]+=1
                    if outcome=='cancel' and calls[0]>=3:raise RuntimeError('Owner cancelled camera preparation before activation')
                def pause(duration):ticks[0]+=duration
                stack.enter_context(mock.patch.object(start,'Resolver',return_value=r))
                stack.enter_context(mock.patch.object(anchor,'build_anchor_config',side_effect=build))
                stack.enter_context(mock.patch.object(anchor,'anchor_ini',return_value='anchor'))
                stack.enter_context(mock.patch.object(start.time,'monotonic',side_effect=lambda:ticks[0]))
                remote=stack.enter_context(mock.patch.object(start,'remote_call'))
                write=stack.enter_context(mock.patch.object(pathlib.Path,'write_text'))
                stack.enter_context(contextlib.redirect_stdout(io.StringIO()))
                def acquire():
                    return start.acquire_observer_preparation(1,{},state,{1:b'ok'},pathlib.Path('Unused.dll'),
                        anchor_observer=True,record_anchor_failure=False,preparation_deadline=.5,check_live=check)
                with self.assertRaisesRegex(RuntimeError if outcome=='cancel' else TimeoutError,
                                            'cancelled' if outcome=='cancel' else 'deadline'):
                    start.wait_camera_preparation(acquire,check,.5,clock=lambda:ticks[0],sleep=pause)
                r.resolve.assert_called_once();r.p.close.assert_called_once()
                self.assertEqual(state.reference,camera);remote.assert_not_called();write.assert_not_called()

    def test_explicit_camera_comparison_is_unchanged(self):
        for key,reason in (('local_player_address','Local player changed'),('module_base','Module base changed')):
            with self.subTest(key=key):
                expected=dict(local_player_address='0x1',module_base='0x1000')
                r=mock.Mock();r.resolve.return_value=dict(expected,**{key:'0x2000'})
                with mock.patch.object(start,'Resolver',return_value=r),mock.patch.object(anchor,'build_anchor_config') as build:
                    with self.assertRaisesRegex(RuntimeError,reason):
                        start.acquire_observer_preparation(1,{},expected,{},pathlib.Path('Unused.dll'),anchor_observer=True)
                self.assertEqual(expected,dict(local_player_address='0x1',module_base='0x1000'))
                r.resolve.assert_called_once();r.p.close.assert_called_once();build.assert_not_called()


if __name__=='__main__':unittest.main()
