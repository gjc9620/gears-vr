import hashlib
from pathlib import Path
import tempfile
from types import SimpleNamespace
import sys
import unittest
from unittest.mock import Mock,patch

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


class PlaygroundInitializationTests(unittest.TestCase):
    class Failure(RuntimeError):
        def __init__(self,code,restored):
            super().__init__('fixture initialization failure')
            self.code=code;self.entry_restored=restored

    def start(self,code,restored):
        resolver=SimpleNamespace(prepare=Mock(),p=SimpleNamespace(close=Mock()))
        bridge=Mock(side_effect=self.Failure(code,restored));attach=Mock();live=Mock()
        manifest=dict(playground_sha256='a'*64)
        modules={'runtime':SimpleNamespace(Bridge=bridge,InitializationError=self.Failure),
                 'resolve_camera':SimpleNamespace(Resolver=Mock(return_value=resolver))}
        with patch.dict(sys.modules,modules),patch.object(p,'artifact',return_value=Path('Range.dll')):
            result=p.start(manifest,{'pid':123},attach=attach,check_live=live)
        attach.assert_not_called();resolver.p.close.assert_called_once()
        return result,live

    def test_resource_failure_continues_only_after_full_entry_restore(self):
        for code in (4,5,6,7):
            with self.subTest(code=code):
                result,live=self.start(code,True)
                self.assertFalse(result['enabled']);self.assertTrue(result['entry_restored'])
                self.assertEqual(result['initialization_code'],code);self.assertEqual(live.call_count,3)

    def test_unknown_or_invalid_engine_identity_still_fails(self):
        for code in (1,2,3,99):
            with self.subTest(code=code),self.assertRaises(self.Failure):self.start(code,True)

    def test_unrestored_or_unknown_entry_never_continues(self):
        for restored in (False,None):
            with self.subTest(restored=restored),self.assertRaises(self.Failure):self.start(5,restored)

    def test_bridge_independently_reads_full_live_entry_after_initialization_failure(self):
        sys.path.insert(0,str(p.ROOT/'tools/playground'))
        from runtime import Bridge,InitializationError
        for raw in (p.ENTRY,b'changed',b''):
            with self.subTest(raw=raw):
                bridge=object.__new__(Bridge);bridge.verify=Mock()
                bridge.r=SimpleNamespace(base=0x140000000,read=Mock(return_value=raw))
                with self.assertRaises(InitializationError) as caught:bridge.initialization_failed(5)
                self.assertEqual(caught.exception.entry_restored,raw==p.ENTRY)
                bridge.r.read.assert_called_once_with(0x140000000+p.ENTRY_RVA,len(p.ENTRY))
        bridge=object.__new__(Bridge);bridge.verify=Mock()
        bridge.r=SimpleNamespace(base=0x140000000,read=Mock(side_effect=OSError('unavailable')))
        with self.assertRaises(InitializationError) as caught:bridge.initialization_failed(5)
        self.assertFalse(caught.exception.entry_restored)


class PlaygroundReservationTests(unittest.TestCase):
    def test_early_reservation_leaves_entry_original_and_closes_only_handles(self):
        bridge=SimpleNamespace(verify=Mock(),close=Mock(),export=Mock(),
            r=SimpleNamespace(base=0x140000000,read=Mock(return_value=p.ENTRY)))
        constructor=Mock(return_value=bridge)
        modules={'runtime':SimpleNamespace(Bridge=constructor,InitializationError=PlaygroundInitializationTests.Failure)}
        with patch.dict(sys.modules,modules),patch.object(p,'artifact',return_value=Path('Range.dll')):
            result=p.reserve({'playground_sha256':'a'*64},{'pid':123})
        self.assertTrue(result['reserved']);self.assertFalse(result['hook_enabled'])
        constructor.assert_called_once_with({'pid':123},Path('Range.dll'),expected_sha256='a'*64,reserve_only=True)
        bridge.export.assert_not_called();bridge.close.assert_called_once()
        bridge.r.read.assert_called_once_with(0x140000000+p.ENTRY_RVA,len(p.ENTRY))

    def test_resource_failure_is_recorded_only_with_original_entry_proof(self):
        failure=PlaygroundInitializationTests.Failure(5,True)
        modules={'runtime':SimpleNamespace(Bridge=Mock(side_effect=failure),InitializationError=type(failure))}
        with patch.dict(sys.modules,modules),patch.object(p,'artifact',return_value=Path('Range.dll')):
            result=p.reserve({'playground_sha256':'a'*64},{'pid':123})
        self.assertFalse(result['reserved']);self.assertTrue(result['entry_restored'])
        failure.entry_restored=False
        with patch.dict(sys.modules,modules),patch.object(p,'artifact',return_value=Path('Range.dll')):
            with self.assertRaises(type(failure)):p.reserve({'playground_sha256':'a'*64},{'pid':123})

    def test_reservation_that_changes_entry_is_rejected_and_cleaned(self):
        bridge=SimpleNamespace(identity={'pid':123},verify=Mock(),close=Mock(),export=Mock(return_value=0),
            control=SimpleNamespace(game_absent=Mock(return_value=False)),
            r=SimpleNamespace(base=0x140000000,read=Mock(side_effect=[b'changed',p.ENTRY])))
        modules={'runtime':SimpleNamespace(Bridge=Mock(return_value=bridge),InitializationError=PlaygroundInitializationTests.Failure)}
        with patch.dict(sys.modules,modules),patch.object(p,'artifact',return_value=Path('Range.dll')):
            with self.assertRaisesRegex(RuntimeError,'reservation changed its original entry'):
                p.reserve({'playground_sha256':'a'*64},{'pid':123})
        bridge.export.assert_called_once_with('PlaygroundShutdown');bridge.close.assert_called_once()

    def test_stopping_before_enable_attaches_without_reserving_or_loading(self):
        bridge=SimpleNamespace(identity={'pid':123},verify=Mock(),close=Mock(),export=Mock(return_value=0),
            control=SimpleNamespace(game_absent=Mock(return_value=False)),
            r=SimpleNamespace(base=0x140000000,read=Mock(return_value=p.ENTRY)))
        constructor=Mock(return_value=bridge)
        modules={'runtime':SimpleNamespace(Bridge=constructor,WindowsControl=Mock(return_value=bridge.control))}
        with patch.dict(sys.modules,modules),patch.object(p,'artifact',return_value=Path('Range.dll')):
            result=p.finish_reservation({'playground_sha256':'a'*64},{'pid':123})
        self.assertTrue(result['passed']);self.assertTrue(result['reservation_only'])
        constructor.assert_called_once_with({'pid':123},Path('Range.dll'),expected_sha256='a'*64,attach_only=True)
        bridge.export.assert_called_once_with('PlaygroundShutdown');bridge.close.assert_called_once()

    def test_exited_reserved_game_is_never_injected_for_cleanup(self):
        constructor=Mock();control=SimpleNamespace(game_absent=Mock(return_value=True))
        modules={'runtime':SimpleNamespace(Bridge=constructor,WindowsControl=Mock(return_value=control))}
        with patch.dict(sys.modules,modules):result=p.finish_reservation({}, {'pid':123})
        self.assertTrue(result['game_exited']);constructor.assert_not_called()


if __name__=='__main__':unittest.main()
