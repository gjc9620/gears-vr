"""Production adapter ownership and manifest trust tests; no live process."""
import copy
import json
from pathlib import Path
import tempfile
from types import SimpleNamespace
import unittest
from unittest import mock

import world_rebind_runtime as runtime
from world_rebind_monitor import PreparedRebind


def config():
    def obj(i):
        return dict(address=i*100,cls=9000,outer=8000,number=0,internal_index=i,name='object'+str(i))
    return dict(roles={name:i for i,name in enumerate(('local_player','controller','camera','level','pawn'))},
        identities=[obj(i+1) for i in range(6)],cinematic=dict(role_world_info=5))


class Tests(unittest.TestCase):
    def adapter(self, directory, crosshair=None):
        path=Path(directory)
        (path/'anchor-observer-config.json').write_text(json.dumps(dict(pid=7,read_only=True,config=config())))
        (path/'capture.ini').write_text('[Capture]\npid=7\nfamily_diagnostics=1\nviewport_diagnostics=1\nview_union_mask_fix=1\nshadow_view=1\nshadow_state=1\nshadow_constructor=1\n',encoding='utf-16')
        if crosshair is not None:
            with (path/'capture.ini').open('a',encoding='utf-16') as output:
                output.write('crosshair_assignment_observer='+str(int(crosshair))+'\n')
        return runtime.RuntimePreparation(path,'capture.dll','a'*64,
            dict(pid=7,creation_time=70,image='game.exe'),dict(pid=8,creation_time=80,image='owner.exe'))

    def test_rejects_missing_cinematic_worldinfo_and_invalid_manifest_digest(self):
        invalid=config();invalid['cinematic']={}
        with self.assertRaises(ValueError):runtime.snapshot(invalid)
        with tempfile.TemporaryDirectory() as directory:
            with self.assertRaisesRegex(ValueError,'manifest SHA256'):
                runtime.RuntimePreparation(directory,'capture.dll','',{}, {})

    def test_startup_crosshair_flag_is_forwarded_only_when_enabled(self):
        for enabled in (None,False,True):
            with self.subTest(enabled=enabled),tempfile.TemporaryDirectory() as directory:
                adapter=self.adapter(directory,crosshair=enabled)
                self.assertEqual(adapter.crosshair_assignment_observer,bool(enabled))
                proof=mock.MagicMock();candidate=SimpleNamespace(anchor_metadata=dict(config=config()))
                with mock.patch.object(runtime,'HookEntryAttestation',return_value=proof),\
                     mock.patch.object(runtime,'build_candidate',return_value=candidate) as build,\
                     mock.patch.object(runtime,'publish_candidate'):
                    adapter.prepare(adapter.baseline,1,3,lambda:None)
                self.assertIs(build.call_args.kwargs['attestation'],proof.__enter__.return_value)
                if enabled:self.assertIs(build.call_args.kwargs['crosshair_assignment_observer'],True)
                else:self.assertNotIn('crosshair_assignment_observer',build.call_args.kwargs)

    def test_observation_closes_rpm_lease_on_success_and_rejection(self):
        with tempfile.TemporaryDirectory() as directory:
            adapter=self.adapter(directory)
            for error in (False,True):
                reader=SimpleNamespace(resolve=mock.Mock(return_value={}),p=SimpleNamespace(close=mock.Mock()))
                with mock.patch.object(runtime,'Resolver',return_value=reader),mock.patch.object(runtime,'build_anchor_config',
                    side_effect=RuntimeError('load changed') if error else None,return_value=(config(),{})):
                    if error:
                        with self.assertRaises(RuntimeError):adapter.observe(lambda:None)
                    else:self.assertEqual(adapter.observe(lambda:None),adapter.baseline)
                reader.p.close.assert_called_once()

    def test_prepare_uses_manifest_digest_and_closes_proof(self):
        with tempfile.TemporaryDirectory() as directory:
            adapter=self.adapter(directory)
            candidate=SimpleNamespace(anchor_metadata=dict(config=config()))
            proof=mock.MagicMock()
            with mock.patch.object(runtime,'HookEntryAttestation',return_value=proof) as attest,\
                 mock.patch.object(runtime,'build_candidate',return_value=candidate),\
                 mock.patch.object(runtime,'publish_candidate') as publish:
                result=adapter.prepare(adapter.baseline,1,3,lambda:None)
                self.assertIsInstance(result,PreparedRebind)
                self.assertEqual(result.path.name,'world-rebind-70-2-3.ini')
                self.assertEqual(attest.call_args.kwargs['expected_dll_sha256'],'a'*64)
                proof.__exit__.assert_called_once()
                publish.assert_called_once_with(candidate,result.path)

    def test_changed_certificate_is_never_published(self):
        with tempfile.TemporaryDirectory() as directory:
            adapter=self.adapter(directory)
            changed=copy.deepcopy(config());changed['identities'][1]['number']=1
            with mock.patch.object(runtime,'HookEntryAttestation',return_value=mock.MagicMock()),\
                 mock.patch.object(runtime,'build_candidate',return_value=SimpleNamespace(anchor_metadata=dict(config=changed))),\
                 mock.patch.object(runtime,'publish_candidate') as publish:
                with self.assertRaisesRegex(RuntimeError,'World changed'):
                    adapter.prepare(adapter.baseline,1,1,lambda:None)
                publish.assert_not_called()


if __name__=='__main__':unittest.main()
