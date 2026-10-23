"""Read-only acquisition failures, leases and owner recovery; synthetic only."""
import contextlib
import io
from pathlib import Path
import tempfile
from types import SimpleNamespace
import unittest
from unittest import mock

import start_capture as start
import anchor_config as anchor
import ui_observer_config as ui
import crosshair_assignment_config as crosshair
from resolve_camera import DiscoveryRegistryChanged
import world_rebind as rebind
import world_rebind_runtime as runtime
from world_rebind_monitor import RetryablePreparation
import test_world_rebind_runtime as adapter_fixtures
from test_world_rebind_monitor import Harness


def known_errors():
    return [anchor.AnchorRegistryChanged('Registry changed',{}),ui.UIRegistryChanged('UI registry changed'),
        crosshair.CrosshairRegistryChanged('Crosshair registry changed'),DiscoveryRegistryChanged('Field chain changed'),
        ui.UIControlledPawnUnavailable({}, {},548),
        crosshair.CrosshairControlledLinkChanged(dict(changes=[dict(field='Weapon')])),
        ui.UIConfigRejected('UI certificate read changed')]


class ChurnRecoveryTests(unittest.TestCase):
    def test_adapter_marks_existing_typed_churn_and_preserves_original_error(self):
        for error in known_errors():
            with self.subTest(kind=type(error).__name__),self.assertRaises(RetryablePreparation)as marked:
                runtime._read_only_acquisition(mock.Mock(side_effect=error))
            self.assertIs(marked.exception.error,error);self.assertEqual(marked.exception.scope,'acquisition_churn')

    def test_real_build_exhaustion_closes_every_reader_and_attestation_without_publication(self):
        error=ui.UIRegistryChanged('UI registry changed')
        self.check_real_build_failure([error,error,error],RetryablePreparation,'acquisition_churn')

    def test_message_only_subclasses_identity_code_and_unknown_reads_stay_hard(self):
        class UnknownLink(crosshair.CrosshairControlledLinkChanged):pass
        class UnknownPawn(ui.UIControlledPawnUnavailable):pass
        class UnknownUI(ui.UIConfigRejected):pass
        errors=[RuntimeError('UI certificate read changed'),ui.UIConfigRejected('UI final identity mismatch'),
            crosshair.CrosshairConfigRejected('Registry header changed'),RuntimeError('Target prologue differs'),
            OSError('UI certificate read changed'),UnknownUI('UI certificate read changed'),
            UnknownLink(dict(changes=[dict(field='Weapon')])),UnknownPawn({}, {},548),
            RuntimeError('Controlled Pawn changed; re-resolve required')]
        for error in errors:
            with self.subTest(kind=type(error).__name__):
                self.assertFalse(start.observer_preparation_churn(error))
                with self.assertRaises(type(error))as raised:runtime._read_only_acquisition(mock.Mock(side_effect=error))
                self.assertIs(raised.exception,error)

    def test_hard_failure_after_known_churn_is_not_hidden_by_earlier_attempts(self):
        changed=ui.UIRegistryChanged('UI registry changed');hard=ui.UIConfigRejected('UI final identity mismatch')
        self.check_real_build_failure([changed,changed,hard],ui.UIConfigRejected)

    def test_late_success_remains_a_terminal_acquisition_deadline(self):
        self.check_real_build_failure([],ui.UIConfigRejected,late_success=True)

    def test_discovery_hint_and_sampler_churn_close_observation_before_retry(self):
        for stage in ('discovery','hint','sampler'):
            with self.subTest(stage=stage),tempfile.TemporaryDirectory()as directory,contextlib.ExitStack()as stack:
                adapter=adapter_fixtures.Tests().adapter(directory);adapter._native_discovery=None
                reader=mock.Mock();reader.resolve.return_value={}
                error=anchor.AnchorRegistryChanged('Registry changed',{})if stage=='sampler'else DiscoveryRegistryChanged('Changed')
                if stage=='discovery':reader.resolve.side_effect=error
                if stage=='hint':adapter._cinematic_class_hint=0x1234;reader.identity.side_effect=error
                stack.enter_context(mock.patch.object(runtime,'Resolver',return_value=reader))
                stack.enter_context(mock.patch.object(runtime,'LiveRegistryAnchorSampler',side_effect=error))
                with self.assertRaises(RetryablePreparation)as raised:adapter.observe(lambda:None)
                self.assertIs(raised.exception.error,error);self.assertEqual(raised.exception.scope,'acquisition_churn')
                reader.p.close.assert_called_once()

    def test_publication_exception_is_outside_read_only_retry_boundary(self):
        with tempfile.TemporaryDirectory()as directory:
            adapter=adapter_fixtures.Tests().adapter(directory);error=ui.UIRegistryChanged('UI registry changed')
            candidate=SimpleNamespace(anchor_metadata=dict(config=adapter_fixtures.config()))
            with mock.patch.object(runtime,'HookEntryAttestation'),\
                 mock.patch.object(runtime,'build_candidate',return_value=candidate),\
                 mock.patch.object(runtime,'publish_candidate',side_effect=error):
                with self.assertRaises(ui.UIRegistryChanged)as raised:adapter.prepare(adapter.baseline,1,1,lambda:None)
            self.assertIs(raised.exception,error)

    def test_owner_keeps_retrying_known_churn_and_commits_one_fresh_package(self):
        for error in known_errors():
            h=Harness();calls=[0]
            def prepare(*args):
                calls[0]+=1
                if calls[0]<=5:return runtime._read_only_acquisition(mock.Mock(side_effect=error))
                return h.prepare(*args)
            with self.subTest(kind=type(error).__name__),h.monitor(prepare=prepare)as monitor:
                for delay in (.05,.1,.15,1.7,.3):
                    monitor.poll(h.status);events=h.finish_worker(monitor)
                    self.assertAlmostEqual(monitor.next_poll-h.now,delay);self.assertEqual(monitor.attempts,0)
                    self.assertEqual(events[0]['retry_scope'],'acquisition_churn');self.assertFalse(h.signals)
                    self.assertEqual(events[0]['reason'],type(error).__name__)
                    h.now=monitor.next_poll
                monitor.poll(h.status);h.finish_worker(monitor)
                self.assertEqual(len(h.builds),1);self.assertEqual(len(h.signals),1)
                h.status.update(world_prepare_requests='1',world_candidate_generation='2');monitor.poll(h.status)
                h.status.update(world_generation='2',world_candidate_generation='0')
                self.assertEqual(monitor.poll(h.status)[0]['event'],'world_rebind_committed')
                self.assertEqual(len(h.signals),2);self.assertEqual(monitor.baseline,h.current)

    def test_known_churn_preserves_previous_hard_failure_budget(self):
        h=Harness();calls=[0]
        def prepare(*args):
            calls[0]+=1
            if calls[0]==1:raise ValueError('hard identity failure')
            return runtime._read_only_acquisition(mock.Mock(side_effect=ui.UIRegistryChanged('Changed')))
        with h.monitor(prepare=prepare)as monitor:
            monitor.poll(h.status);h.finish_worker(monitor);self.assertEqual(monitor.attempts,1)
            for _ in range(4):
                h.now=monitor.next_poll;monitor.poll(h.status);h.finish_worker(monitor)
                self.assertEqual(monitor.attempts,1);self.assertFalse(h.signals)

    def test_retry_scope_is_bounded_and_timestamp_default_is_preserved(self):
        error=ValueError('fixture')
        self.assertEqual(RetryablePreparation(error).scope,'timestamp_sample')
        for scope in (None,1,[], 'unknown'):
            with self.subTest(scope=scope),self.assertRaises(ValueError):RetryablePreparation(error,scope=scope)

    def check_real_build_failure(self,errors,kind,scope=None,late_success=False):
        with tempfile.TemporaryDirectory()as directory,contextlib.ExitStack()as stack:
            adapter=adapter_fixtures.Tests().adapter(directory);adapter._native_discovery=None;adapter.signatures={1:b'ok'}
            game=dict(adapter.game);owner=dict(adapter.owner);readers=[];ticks=[0.]
            proof=SimpleNamespace(pid=game['pid'],game_creation=game['creation_time'],owner=owner,dll=adapter.dll,
                module_base=0x1000,reverify=mock.Mock(),read_entry=lambda address,size,reader:reader(address,size))
            lease=mock.MagicMock();lease.__enter__.return_value=proof
            camera=dict(local_player_address=hex(adapter.local['address']),module_base='0x1000',camera_address='0x3000')
            def reader(*args):
                r=mock.Mock();r.base=0x1000;r.resolve.return_value=dict(camera);r.read.return_value=b'ok'
                readers.append(r);return r
            def build_ui(*args,**kwargs):
                if late_success:ticks[0]=11;return {},{}
                raise errors[len(readers)-1]
            stack.enter_context(mock.patch.object(runtime,'HookEntryAttestation',return_value=lease))
            stack.enter_context(mock.patch.object(rebind,'process_identity',side_effect=lambda pid:game if pid==game['pid']else owner))
            stack.enter_context(mock.patch.object(start,'Resolver',side_effect=reader))
            stack.enter_context(mock.patch.object(anchor,'build_anchor_config',side_effect=lambda *a,**k:({},{})))
            stack.enter_context(mock.patch.object(anchor,'anchor_ini',return_value='anchor'))
            stack.enter_context(mock.patch.object(ui,'build_ui_observer_config',side_effect=build_ui))
            stack.enter_context(mock.patch.object(ui,'ui_observer_ini',return_value='ui'))
            stack.enter_context(mock.patch.object(start.time,'monotonic',side_effect=lambda:ticks[0]))
            stack.enter_context(mock.patch.object(start.time,'sleep'))
            publish=stack.enter_context(mock.patch.object(runtime,'publish_candidate'))
            activate=stack.enter_context(mock.patch.object(start,'remote_call'))
            stack.enter_context(contextlib.redirect_stderr(io.StringIO()))
            with self.assertRaises(kind)as raised:adapter.prepare(adapter.baseline,1,1,lambda:None)
            if scope is not None:
                self.assertEqual(raised.exception.scope,scope);self.assertIs(raised.exception.error,errors[-1])
            elif not late_success:self.assertIs(raised.exception,errors[-1])
            self.assertEqual(len(readers),1 if late_success else len(errors))
            for r in readers:r.p.close.assert_called_once();r.resolve.assert_called_once()
            lease.__exit__.assert_called_once();publish.assert_not_called();activate.assert_not_called()


if __name__=='__main__':unittest.main()
