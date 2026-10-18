"""Offline build/receipt publication tests; no executables or audio APIs run."""
import tempfile
from pathlib import Path
from unittest import mock
import unittest
import build_helper

class BuildTests(unittest.TestCase):
    def test_build_then_pure_tests_then_receipt_and_no_helper_execution(self):
        with tempfile.TemporaryDirectory() as tmp:
            root=Path(tmp);source=root/'source';source.mkdir()
            for name in build_helper.SOURCES:(source/name).write_text('source')
            build=root/'new-build';output=root/'output';calls=[]
            def run(command,check):
                calls.append(command)
                build.mkdir(exist_ok=True)
                (build/'CMakeCache.txt').write_text('CMAKE_CXX_COMPILER:FILEPATH=fixture-cl\n')
                (build/'GearsTestAudioGuard.exe').write_bytes(b'fixture-helper')
            with mock.patch.object(build_helper.subprocess,'run',side_effect=run):
                receipt=build_helper.build(source,build,output,root/'cmake.exe','Ninja',root/'ninja.exe')
                with self.assertRaises(FileExistsError):build_helper.build(source,root/'other-build',output,root/'cmake.exe','Ninja')
            self.assertEqual(len(calls),3);self.assertEqual(Path(calls[-1][0]).name,'ctest.exe')
            self.assertTrue(receipt['pure_tests_passed']);self.assertFalse(receipt['helper_executed'])
            self.assertEqual(receipt['helper_sha256'],build_helper.sha(output/'GearsTestAudioGuard.exe'))
            self.assertEqual(set(receipt['source_sha256']),set(build_helper.SOURCES))

    def test_changed_source_rejects_before_publish(self):
        with tempfile.TemporaryDirectory() as tmp:
            root=Path(tmp);source=root/'source';source.mkdir()
            for name in build_helper.SOURCES:(source/name).write_text('source')
            build=root/'build';output=root/'output'
            def run(command,check):
                build.mkdir(exist_ok=True)
                if Path(command[0]).name=='ctest.exe':(source/build_helper.SOURCES[0]).write_text('changed')
            with mock.patch.object(build_helper.subprocess,'run',side_effect=run),self.assertRaisesRegex(RuntimeError,'sources changed'):
                build_helper.build(source,build,output,root/'cmake.exe','Ninja')
            self.assertFalse(output.exists())
if __name__=='__main__':unittest.main()
