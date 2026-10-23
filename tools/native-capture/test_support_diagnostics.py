"""Fault journals and process identity checks using files/fake providers only."""
import json
from pathlib import Path
import tempfile
import unittest
from unittest import mock
from support_diagnostics import SupportJournal, message


class Provider:
    def __init__(self):self.closed=False
    def process(self,identity):return dict(available=True,state='running',exit_code=None)
    def modules(self,identity):return dict(available=True,modules=[dict(module_name='openxr_loader.dll',file_version='1.2.3.4')])
    def software(self):return dict(available=True,software=[dict(component='steamvr',version='2.0')])
    def close(self):self.closed=True


class SupportTests(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory();self.addCleanup(self.temp.cleanup)
        self.root=Path(self.temp.name);self.prefix=self.root/'continuous-stereo-123-20261024-010000-abcdef01'
        self.build=self.root/'build';self.build.mkdir();self.game=dict(pid=123,creation_time=1230)
        self.journal=SupportJournal(self.prefix,self.game,self.build,owner=dict(pid=456,creation_time=4560))

    def rows(self):return [json.loads(line) for line in self.journal.path.read_text().splitlines()]

    def test_first_failure_is_durable_and_survives_cleanup_error(self):
        self.journal.failure('controller','native_activation','RuntimeError: original failure')
        first=self.journal.first.read_bytes()
        self.journal.failure('controller','cleanup','Later cleanup failed')
        self.assertEqual(self.journal.first.read_bytes(),first)
        self.assertEqual(len(self.rows()),2)
        self.assertTrue(self.rows()[0]['first_observed_failure']);self.assertFalse(self.rows()[1]['first_observed_failure'])
        self.assertEqual(json.loads(first)['category'],'native_activation')

    def test_identity_bound_native_and_xr_adapter_evidence(self):
        self.journal.bind_xr(dict(pid=789,creation_time=7890))
        def row(pid,born,low):return dict(support_diagnostics=True,schema=1,event='adapter',pid=pid,creation_time=born,adapter_luid_high=1,adapter_luid_low=low)
        native=self.build/'support-native-123.jsonl'
        native.write_text(json.dumps(row(123,111,99))+'\n'+json.dumps(row(123,1230,2))+'\n')
        self.prefix.with_suffix('.stdout.txt').write_text(json.dumps(row(789,7890,2))+'\n')
        self.journal.tick(Provider())
        rows=self.rows();self.assertTrue(any(r.get('adapter_match') is True for r in rows))
        self.assertFalse(any(r.get('adapter_luid_low')==99 for r in rows))
        count=len(rows);self.journal.tick(Provider())
        self.assertEqual(sum(r.get('event')=='adapter' for r in self.rows()),2)

    def test_live_status_requires_fresh_owner_identity_and_survives_process_loss(self):
        source=self.build/'session-status.txt'
        source.write_text('pid=123 owner_pid=456 owner_creation_time=111 complete=1 published=0')
        self.journal.tick(Provider());self.assertFalse(self.prefix.with_suffix('.native-live-status.txt').exists())
        source.write_text('pid=123 owner_pid=456 owner_creation_time=4560 complete=1 published=0 telemetry_present_calls=1')
        self.journal.tick(Provider())
        saved=self.prefix.with_suffix('.native-live-status.txt').read_text()
        source.unlink();self.journal.tick(Provider())
        self.assertEqual(self.prefix.with_suffix('.native-live-status.txt').read_text(),saved)
        self.assertTrue(any(r.get('event')=='pipeline_progress' and r['counters']['published']==0 for r in self.rows()))

    def test_observer_error_is_recorded_without_control_side_effects(self):
        provider=Provider();provider.software=mock.Mock(side_effect=PermissionError())
        journal=SupportJournal(self.prefix,self.game,self.build,provider_factory=lambda:provider).start()
        result=journal.close()
        self.assertTrue(result['worker_joined']);self.assertEqual(result['observer_error'],'PermissionError');self.assertTrue(provider.closed)
        self.assertFalse(journal.first.exists())

    def test_private_paths_and_credentials_are_removed_from_failure_text(self):
        text=message('token=PRIVATE user@example.com failed at C:\\Users\\PrivateUser\\file.log')
        self.assertNotIn('PrivateUser',text);self.assertNotIn('user@example.com',text);self.assertNotIn('token=PRIVATE',text)

    def test_multiple_rotations_retain_device_context_with_original_time(self):
        system=self.journal.write('xr','system',system_name='Runtime supplied system')
        self.journal.write('native','adapter',chain_id=2,adapter_luid_high=1,adapter_luid_low=2)
        self.journal.write('xr','adapter',adapter_luid_high=1,adapter_luid_low=2)
        self.journal.write('environment','software_inventory',available=True,software=[])
        for _ in range(3):
            self.journal.path.write_bytes(b'x'*(2*1024*1024+1))
            self.journal.write('game','process_state',available=True,state='running')
        rows=self.rows()
        self.assertEqual(self.journal.rotations,3)
        self.assertIn(system,rows)
        self.assertEqual(len([r for r in rows if r['event']=='adapter']),2)
        self.assertTrue(any(r['event']=='software_inventory' for r in rows))
        self.assertEqual(rows[-1]['event'],'process_state')


if __name__=='__main__':unittest.main()
