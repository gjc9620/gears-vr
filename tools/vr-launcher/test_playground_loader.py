import hashlib
from pathlib import Path
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import Mock

import playground_loader as p
import stage_portable
import portable_preflight


class PlaygroundArtifactTests(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory();self.addCleanup(self.temp.cleanup)
        self.root=Path(self.temp.name);self.file=self.root/'tools/playground/build-test/Range.dll'
        self.file.parent.mkdir(parents=True);self.file.write_bytes(b'fixture-not-a-real-dll')
        self.manifest=dict(playground_menu=True,playground_build='tools/playground/build-test',playground_dll='Range.dll',
                           playground_sha256=hashlib.sha256(self.file.read_bytes()).hexdigest())
    def test_matching_artifact(self):
        self.assertEqual(p.artifact(self.manifest,self.root),self.file.resolve())
    def test_changed_dll_rejected_before_injection(self):
        self.file.write_bytes(b'changed')
        with self.assertRaisesRegex(RuntimeError,'SHA256 mismatch'):p.artifact(self.manifest,self.root)
    def test_traversal_and_unrelated_artifacts(self):
        for build,dll in [('../outside','Range.dll'),('tools/playground/../outside','Range.dll'),
                          ('tools/native-capture/build','Range.dll'),('tools/playground/build','../Range.dll')]:
            with self.subTest(build=build,dll=dll),self.assertRaises(ValueError):
                p.relative_artifact(dict(self.manifest,playground_build=build,playground_dll=dll))
    def test_declared_component_is_in_portable_payload(self):
        manifest=dict(self.manifest,native_build='tools/native-capture/build',native_dll='Native.dll',xr_build='tools/openxr-probe/build')
        files=stage_portable.payload_paths(manifest)
        self.assertIn('tools/playground/build-test/Range.dll',files)
        self.assertIn('tools/playground/runtime.py',files)
        self.assertIn('tools/vr-launcher/playground_loader.py',files)
        self.assertFalse(any(f.startswith('analysis/') for f in files))
    def test_injected_component_has_only_verified_system_loader_dependencies(self):
        manifest=dict(self.manifest,native_build='tools/native-capture/build',native_dll='Native.dll',xr_build='tools/openxr-probe/build')
        system=self.root/'system';system.mkdir();(system/'kernel32.dll').write_bytes(b'fixture')
        (self.file.parent/'vendor.dll').write_bytes(b'fixture')
        result=portable_preflight.resolve_pe_dependency(self.root,self.file,'kernel32.dll',manifest,system)
        self.assertEqual(result['scope'],'windows-system32')
        with self.assertRaisesRegex(ValueError,'Unresolved PE import'):
            portable_preflight.resolve_pe_dependency(self.root,self.file,'vendor.dll',manifest,system)
        other=self.file.parent/'Other.dll';other.write_bytes(b'fixture')
        with self.assertRaisesRegex(ValueError,'Unknown PE loader context'):
            portable_preflight.resolve_pe_dependency(self.root,other,'kernel32.dll',manifest,system)


class PlaygroundCleanupTests(unittest.TestCase):
    def bridge(self,raw):
        return SimpleNamespace(identity={'pid':123},control=SimpleNamespace(game_absent=Mock(return_value=False)),
            verify=Mock(),export=Mock(return_value=0),r=SimpleNamespace(base=0x140000000,read=Mock(return_value=raw)),close=Mock())
    def test_stop_requires_independently_restored_entry(self):
        bridge=self.bridge(b'wrong')
        self.assertFalse(p.finish(bridge)['passed']);bridge.close.assert_called_once()
        bridge.export.assert_called_once_with('PlaygroundShutdown')
    def test_original_entry_accepted_after_stop(self):
        bridge=self.bridge(p.ENTRY);self.assertTrue(p.finish(bridge)['passed'])
        bridge.r.read.assert_called_once_with(0x140000000+p.ENTRY_RVA,len(p.ENTRY))
    def test_exited_game_is_not_injected_again(self):
        bridge=self.bridge(b'');bridge.control.game_absent.return_value=True
        self.assertTrue(p.finish(bridge)['game_exited']);bridge.export.assert_not_called();bridge.close.assert_called_once()


if __name__=='__main__':unittest.main()
