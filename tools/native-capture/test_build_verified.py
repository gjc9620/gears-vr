"""Pure dependency validation fixtures; no compiler/build processes executed."""
from pathlib import Path
import tempfile
from types import SimpleNamespace
import unittest
from unittest import mock
import build_verified as build

class BuildTests(unittest.TestCase):
    def test_language_preference_applied_after_vcvars_not_assumed_sufficient(self):
        with tempfile.TemporaryDirectory()as temp:
            vc=Path(temp)/'vcvars64.bat';vc.write_text('fixture')
            run=mock.Mock(return_value=SimpleNamespace(stdout='Path=fixture\nVSLANG=2052\n'.encode('utf-16-le')))
            env=build.vc_environment(vc,run=run)
            self.assertEqual(env['VSLANG'],'1033');self.assertEqual(env['PYTHONUTF8'],'1')
            self.assertEqual(run.call_count,1)

    def test_zero_deps_and_missing_required_header_are_terminal(self):
        for text in ('object: #deps 0, VALID\n','missing object'):
            with self.assertRaisesRegex(RuntimeError,'zero/missing'):build.dependency_proof(text,['host.inl'])
        with self.assertRaisesRegex(RuntimeError,'not tracked'):
            build.dependency_proof('object: #deps 1, VALID\n    j:/other.h\n',['host.inl'])
        self.assertEqual(build.dependency_proof('object: #deps 1, VALID\n    J:/SRC/HOST.INL\n',['host.inl'])['count'],1)

    def test_generated_prefix_must_equal_actual_raw_utf8(self):
        with tempfile.TemporaryDirectory()as temp:
            root=Path(temp);(root/'CMakeFiles').mkdir();compiler=root/'cl.exe'
            (root/'CMakeCache.txt').write_text('CMAKE_CXX_COMPILER:FILEPATH='+str(compiler)+'\n')
            expected='注意: 包含文件:  '
            rules=root/'CMakeFiles/rules.ninja';rules.write_text('msvc_deps_prefix = '+expected+'\n',encoding='utf-8')
            self.assertEqual(build.verified_prefix(root,expected,compiler),expected)
            rules.write_text('msvc_deps_prefix = mojibake\n',encoding='utf-8')
            with self.assertRaisesRegex(RuntimeError,'differs'):build.verified_prefix(root,expected,compiler)

    def test_existing_build_is_rejected_before_environment_or_compilation(self):
        with tempfile.TemporaryDirectory()as temp,mock.patch.object(build,'vc_environment')as env:
            with self.assertRaises(FileExistsError):
                build.build_verified(temp,temp,'cmake','ninja','vcvars',[],['GearsCapture'],'file.obj',['host.inl'])
            env.assert_not_called()

    def test_different_cmake_compiler_rejected(self):
        with tempfile.TemporaryDirectory()as temp:
            root=Path(temp);(root/'CMakeFiles').mkdir()
            (root/'CMakeFiles/rules.ninja').write_text('msvc_deps_prefix = prefix\n')
            (root/'CMakeCache.txt').write_text('CMAKE_CXX_COMPILER:FILEPATH='+str(root/'other-cl.exe')+'\n')
            with self.assertRaisesRegex(RuntimeError,'compiler differs'):build.verified_prefix(root,'prefix',root/'cl.exe')
if __name__=='__main__':unittest.main()
